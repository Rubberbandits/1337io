#ifndef __HEADER_CFILE__
#define __HEADER_CFILE__
#pragma once

#include "Main.h"
#include <fstream>
#include <sstream>

enum FILE_OPERATION {
	FILE_OK,
	FILE_NULLPTR,
	FILE_MISSING_ARGUMENTS,
	FILE_CANT_OPEN,
	FILE_ALREADY_OPEN,
	FILE_NOT_OPEN,
};

#define TYPE_FILE 1

class CFile {
public:
	CFile(lua_State* state);
	~CFile();

	static int gcDeleteWrapper(lua_State* state);

	void CreateMetaTable(lua_State* state);
	void CreateNewInstance(lua_State* state);

	void OnRemove(lua_State* state);

	// raw

	void DeleteFileName();
	void SetFile(const char* pszFileName);
	const char* GetFile();

	FILE_OPERATION _Open(const char* pszFileName, bool bNoOverride = false);
	FILE_OPERATION _Close();

	FILE_OPERATION _Write(const char* pszText);
	FILE_OPERATION _Read(std::stringstream& rStringStream);
	FILE_OPERATION _Flush();
	FILE_OPERATION _Clear();

	FILE_OPERATION _Length(long& rnSize);

	// functions exposed to lua

	static CFile* UnpackMe(lua_State* state); // helper

	static int Close(lua_State* state);

	static int Write(lua_State* state);
	static int Append(lua_State* state);
	static int Read(lua_State* state);
	static int Flush(lua_State* state);
	static int Clear(lua_State* state);

	static int Length(lua_State* state);
private:
	std::ofstream m_ofFile;
	char* m_pszFileName;
};

#endif