#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

class SigScanner
{
public:
	SigScanner() {};
	MODULEINFO GetModuleInfo(char* szModule);
	void* FindPattern(const char module[], const char pattern[], const char mask[]);
	void* FindPattern(HMODULE module, const char* pattern, const char* mask);
};
