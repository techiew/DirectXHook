#pragma once

#include <vector>
#include "SigScanner.h"
#include "DebugConsole.h"

class MemReader
{
private:
	DebugConsole* console;

public:
	MemReader() {};
	MemReader(DebugConsole* console);
	bool ReadMemory(void* address, void* buffer, size_t numBytes);
};