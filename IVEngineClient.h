#ifndef __HEADER_IVENGINECLIENT__
#define __HEADER_IVENGINECLIENT__
#pragma once

class IVEngineClient {
public:
	const char* GetGameDirectory() {
		using GetGameDirectoryFn = const char*(__thiscall*)(IVEngineClient*);
		static auto GetGameDirectory = (*reinterpret_cast<GetGameDirectoryFn**>(this))[35]; // it index 35 ok

		return GetGameDirectory(this);
	}
};

extern IVEngineClient* g_pEngine;

#endif