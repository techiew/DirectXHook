#include "SigScanner.h"

// I used this tutorial to make this, all credit to Guided Hacking
//https://www.youtube.com/watch?v=S_SR5l_hquw

MODULEINFO SigScanner::GetModuleInfo(char* szModule)
{
	MODULEINFO modInfo = { 0 };
	HMODULE hModule = GetModuleHandle(szModule);

	if (hModule == 0) return modInfo;

	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

	return modInfo;
}

void* SigScanner::FindPattern(const char module[], const char pattern[], const char mask[])
{
	MODULEINFO modInfo = GetModuleInfo((char*)module);

	long long base = (long long)modInfo.lpBaseOfDll;
	DWORD size = modInfo.SizeOfImage;

	unsigned int patternLength = strlen(mask);

	for (long long i = 0; i < size - patternLength; i++)
	{
		bool found = true;

		for (long long j = 0; j < patternLength; j++)
		{
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		if (found) return (void*)((char*)(base + i));
	}

	return 0;
}

// This one is used to scan the original dxgi.dll directly, using the HMODULE reference we stored after we loaded it.
// We can't target it by name, since the name of our .dll is the same we will end up targeting our own .dll instead.
void* SigScanner::FindPattern(HMODULE module, const char* pattern, const char* mask)
{
	MODULEINFO modInfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), (HMODULE)module, &modInfo, sizeof(MODULEINFO));

	long long base = (long long)modInfo.lpBaseOfDll;
	DWORD size = modInfo.SizeOfImage;

	unsigned int patternLength = strlen(mask);

	for (long long i = 0; i < size - patternLength; i++)
	{
		bool found = true;

		for (long long j = 0; j < patternLength; j++)
		{
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		if (found) return (void*)((char*)(base + i));
	}

	return 0;
}