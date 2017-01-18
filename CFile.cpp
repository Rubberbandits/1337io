#include "CFile.h"

//note: i got lazy in the end
//      will fix soon

CFile::CFile(lua_State* state) {
	CreateNewInstance(state);
}

CFile::~CFile() {

}

int CFile::gcDeleteWrapper(lua_State* state) {
	using namespace GarrysMod::Lua;

	UserData* pUserData = static_cast<UserData*>(LUA->GetUserdata(1));
	if (pUserData == nullptr || pUserData->type != TYPE_FILE) return 0;

	auto pFile = static_cast<CFile*>(pUserData->data);
	if (pFile == nullptr) return 0;

	pFile->OnRemove(state);
	delete pFile;

	return 0;
}

static int iUserMeta = 0;
void CFile::CreateMetaTable(lua_State* state){
	LUA->CreateTable();
		LUA->PushCFunction(gcDeleteWrapper);
		LUA->SetField(-2, "__gc");
	iUserMeta = LUA->ReferenceCreate();
}

void CFile::CreateNewInstance(lua_State* state) {
	using namespace GarrysMod::Lua;
	if (iUserMeta == 0) CreateMetaTable(state);

	UserData* pUserData = static_cast<UserData*>(LUA->NewUserdata(sizeof(UserData)));
	pUserData->data = this;
	pUserData->type = TYPE_FILE;

	int iReference = LUA->ReferenceCreate();
	
	LUA->ReferencePush(iReference);
		LUA->ReferencePush(iUserMeta);
		LUA->SetMetaTable(-2);
	LUA->Pop();

	LUA->CreateTable();
		LUA->PushCFunction(Close);
		LUA->SetField(-2, "Close");

		LUA->PushCFunction(Write);
		LUA->SetField(-2, "Write");

		LUA->PushCFunction(Append);
		LUA->SetField(-2, "Append");

		LUA->PushCFunction(Read);
		LUA->SetField(-2, "Read");

		LUA->PushCFunction(Flush);
		LUA->SetField(-2, "Flush");

		LUA->PushCFunction(Clear);
		LUA->SetField(-2, "Clear");

		LUA->PushCFunction(Length);
		LUA->SetField(-2, "Length");

		LUA->ReferencePush(iReference);
		LUA->SetField(-2, "__usrdata_ref");
	LUA->ReferenceFree(iReference);
}

void CFile::OnRemove(lua_State* state) {
	_Close();
	DeleteFileName();
}

/* raw functions */

void CFile::DeleteFileName() {
	if (m_pszFileName == nullptr) return;

	delete[] m_pszFileName;
	m_pszFileName = nullptr;
}

void CFile::SetFile(const char* pszFileName) {
	DeleteFileName();

	m_pszFileName = new char[strlen(pszFileName) + 1];
	strcpy(m_pszFileName, pszFileName);
}

const char* CFile::GetFile() {
	return m_pszFileName;
}

FILE_OPERATION CFile::_Open(const char* pszFileName, bool bNoOverride) {
	if (m_ofFile.is_open()) return FILE_ALREADY_OPEN;
	
	m_ofFile.open(pszFileName, std::ios_base::out | std::ios_base::app);
	if (!m_ofFile.is_open()) return FILE_CANT_OPEN; // lol im lazy

	if (!bNoOverride) {
		SetFile(pszFileName);
	}

	return FILE_OK;
}

FILE_OPERATION CFile::_Close() {
	if (!m_ofFile.is_open()) return FILE_NOT_OPEN;

	//_Flush();
	m_ofFile.close();

	return FILE_OK;
}

FILE_OPERATION CFile::_Write(const char* pszText) {
	if (!m_ofFile.is_open()) return FILE_NOT_OPEN;

	m_ofFile << pszText;

	return FILE_OK;
}

FILE_OPERATION CFile::_Read(std::stringstream& rStringStream) {
	if (!m_ofFile.is_open()) return FILE_NOT_OPEN;

	auto _result = _Close();
	if (_result != FILE_OK) return _result;

	std::ifstream tempstream; // this from stackoverflow
	tempstream.open(GetFile(), std::ios_base::in);
	if (!tempstream.is_open()) return FILE_NOT_OPEN;
	rStringStream << tempstream.rdbuf();
	tempstream.close();

	_result = _Open(GetFile(), true);

	return _result;
}

FILE_OPERATION CFile::_Flush() {
	if (!m_ofFile.is_open()) return FILE_NOT_OPEN;

	m_ofFile.flush();

	return FILE_OK;
}

FILE_OPERATION CFile::_Clear() {
	if (!m_ofFile.is_open()) return FILE_NOT_OPEN;

	auto _result = _Close();
	if (_result != FILE_OK) return _result;

	std::ofstream tempstream;
	tempstream.open(GetFile(), std::ios_base::out | std::ios_base::trunc);
	tempstream.close();

	_result = _Open(GetFile(), true);

	return FILE_OK;
}

FILE_OPERATION CFile::_Length(long& rnSize) {
	if (!m_ofFile.is_open()) return FILE_NOT_OPEN;

	auto _result = _Close();
	if (_result != FILE_OK) return _result;

	std::ifstream tempstream; // this from stackoverflow
	tempstream.open(GetFile(), std::ios_base::in | std::ios_base::binary);
	if (!tempstream.is_open()) return FILE_NOT_OPEN;
	tempstream.ignore(std::numeric_limits<std::streamsize>::max());
	rnSize = tempstream.gcount();
	tempstream.close();

	_result = _Open(GetFile(), true);

	return _result;
}

/* funcs for lua */

CFile* CFile::UnpackMe(lua_State* state) {
	using namespace GarrysMod::Lua;

	LUA->GetField(1, "__usrdata_ref");
	UserData* pUserData = static_cast<UserData*>(LUA->GetUserdata(-1));
	LUA->Pop();

	if (pUserData == nullptr || pUserData->type != TYPE_FILE) return nullptr;

	auto pFile = static_cast<CFile*>(pUserData->data);
	if (pFile == nullptr) return nullptr;

	return pFile;
}

int CFile::Close(lua_State* state) {
	FILE_OPERATION _result = FILE_NULLPTR;
	auto pFile = UnpackMe(state);

	if (pFile != nullptr) _result = pFile->_Close();

	LUA->PushNumber(_result);
	return 1;
}

int CFile::Write(lua_State* state) {
	using namespace GarrysMod::Lua;

	FILE_OPERATION _result = FILE_NULLPTR;
	auto pFile = UnpackMe(state);

	if (pFile != nullptr) {
		if (!LUA->IsType(2, Type::STRING)) {
			LUA->ThrowError("NO STRING TO WRITE");
			_result = FILE_MISSING_ARGUMENTS;
		}
		else {
			_result = pFile->_Clear();
			if (_result != FILE_OK) goto breakout;

			_result = pFile->_Write(LUA->GetString(2));
		}
	}

	breakout:
	LUA->PushNumber(_result);
	return 1;
}

int CFile::Append(lua_State* state) {
	using namespace GarrysMod::Lua;

	FILE_OPERATION _result = FILE_NULLPTR;
	auto pFile = UnpackMe(state);

	if (pFile != nullptr) {
		if (!LUA->IsType(2, Type::STRING)) {
			LUA->ThrowError("NO STRING TO WRITE");
			_result = FILE_MISSING_ARGUMENTS;
		}
		else {
			_result = pFile->_Write(LUA->GetString(2));
		}
	}

	LUA->PushNumber(_result);
	return 1;
}

int CFile::Read(lua_State* state) {
	FILE_OPERATION _result = FILE_NULLPTR;
	std::stringstream tempstream;
	auto pFile = UnpackMe(state);

	if (pFile != nullptr) _result = pFile->_Read(tempstream);

	LUA->PushString(tempstream.str().c_str());
	LUA->PushNumber(_result);
	return 2;
}

int CFile::Flush(lua_State* state) {
	FILE_OPERATION _result = FILE_NULLPTR;
	auto pFile = UnpackMe(state);

	if (pFile != nullptr) _result = pFile->_Flush();

	LUA->PushNumber(_result);
	return 1;
}

int CFile::Clear(lua_State* state) { // HACKISH??
	FILE_OPERATION _result = FILE_NULLPTR;
	auto pFile = UnpackMe(state);

	if (pFile != nullptr) _result = pFile->_Clear();

	LUA->PushNumber(_result);
	return 1;
}

int CFile::Length(lua_State* state) {
	FILE_OPERATION _result = FILE_NULLPTR;
	long nSize = -1;
	auto pFile = UnpackMe(state);

	if (pFile != nullptr) _result = pFile->_Length(nSize);

	LUA->PushNumber(nSize);
	LUA->PushNumber(_result);
	return 2;
}