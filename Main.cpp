#include "Main.h"

IVEngineClient* g_pEngine = nullptr;

template< typename Interface >
static Interface* GetInterface(const char* pszInterfaceName, const char* pszModule) { // bruteforce weil lazy bro
	Interface* pTemp = nullptr;
	int nCurrentVersion = 0;

	using CreateInterfaceFn = Interface*(*)(const char*, int);
	CreateInterfaceFn TempFactory = reinterpret_cast<CreateInterfaceFn>(
		GetProcAddress(GetModuleHandleA(pszModule), "CreateInterface")
	);

	do {
		char szBuf[64] = { 0 };
		sprintf_s(szBuf, "%s%03d", pszInterfaceName, nCurrentVersion);

		pTemp = TempFactory(szBuf, 0);
	} while (pTemp == nullptr && ++nCurrentVersion < 100);

	return pTemp;
}

static bool IsPathSafe(const char* pszPathName) { // will do this better later i promise
	char szBuf[MAX_PATH + 1 + 3]; // copy so we can be cool and replace shit
	memset(szBuf, 0, sizeof(szBuf));
	strcpy_s(szBuf, pszPathName);

	for (int i = 0; szBuf[i]; i++) {
		if (szBuf[i] == '\\')
			szBuf[i] = '/';
		else if (szBuf[i] == ':')
			return false;
	}

	if (szBuf[0] == '/') return false;

	for (int i = 0; szBuf[i]; i++) {
		if (szBuf[i] == '.' && szBuf[i + 1] == '.' && szBuf[i + 2] == '/')
			return false; // no ../ sorry folkz
	}

	return true;
}

static int Open(lua_State* state) {
	using namespace GarrysMod::Lua;

	if (!LUA->IsType(1, Type::STRING)) {
		LUA->ThrowError("No file name provided");
		return 0;
	}

	const char* pszFileName = LUA->GetString(1);
	if (!IsPathSafe(pszFileName)) {
		LUA->ThrowError("File path not safe");
		return 0;
	}

	char szFmtFileName[MAX_PATH + 1] = { 0 };
	sprintf_s(szFmtFileName, "%s/%s", g_pEngine->GetGameDirectory(), pszFileName);

	CFile* pFile = new CFile(state);
	pFile->_Open(szFmtFileName);
	return 1;
}

static int GetGameDirectory(lua_State* state) {
	LUA->PushString(g_pEngine->GetGameDirectory());

	return 1;
}

#undef CreateDirectory

static int CreateDirectory(lua_State* state) { // idk wtf im doing 
	using namespace GarrysMod::Lua;

	if (!LUA->IsType(1, Type::STRING)) {
		LUA->ThrowError("No file name provided");
		return 0;
	}

	const char* pszFolderPath = LUA->GetString(1);
	if (!IsPathSafe(pszFolderPath)) {
		LUA->ThrowError("Folder path not safe");
		return 0;
	}

	char szFmtFolderPath[MAX_PATH + 1] = { 0 };
	sprintf_s(szFmtFolderPath, "%s/%s", g_pEngine->GetGameDirectory(), pszFolderPath);
	int nLen = strlen(szFmtFolderPath);
	if (szFmtFolderPath[nLen - 1] != '/' && szFmtFolderPath[nLen - 1] != '\\') {
		szFmtFolderPath[nLen] = '\\';
		szFmtFolderPath[nLen + 1] = '\0';
	}

	for (int i = 0; szFmtFolderPath[i]; i++)
		if (szFmtFolderPath[i] == '/')
			szFmtFolderPath[i] = '\\';

	MakeSureDirectoryPathExists(szFmtFolderPath); // rifk
	return 0;
}

GMOD_MODULE_OPEN() {
	g_pEngine = GetInterface<IVEngineClient>("VEngineClient", "engine.dll");

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->CreateTable();
			LUA->PushCFunction(Open);
			LUA->SetField(-2, "Open");

			LUA->PushCFunction(CreateDirectory);
			LUA->SetField(-2, "CreateDirectory");

			LUA->PushCFunction(GetGameDirectory);
			LUA->SetField(-2, "GetDirectory");

			// DELETE FEATURE COMING SOON ONCE I FIX IsPathSafe AND STUFF!!!
		LUA->SetField(-2, "leetio");
	LUA->Pop();

	return 0;
}

GMOD_MODULE_CLOSE() {
	return 0;
}