#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

#ifdef _WIN64
#define _WIN64 1
#endif

#ifdef _WIN64
typedef unsigned __int64 QWORD; // My C++ doesn't have QWORD for some reason
typedef QWORD MEMADDR;
#else
typedef DWORD MEMADDR;
#endif

class SigScanner
{
public:
	SigScanner();
	MODULEINFO GetModuleInfo(char* szModule);
	void* FindPattern(const char module[], const char pattern[], const char mask[]);
	void* FindPattern(HMODULE module, const char* pattern, const char* mask);
};
