#include "MemReader.h"

MemReader::MemReader(DebugConsole* console)
{
	this->console = console;
	console->PrintDebugMsg("Base address of process: %p", GetModuleHandleA(NULL));
}

bool MemReader::ReadMemory(void* address, void* buffer, size_t numBytes)
{
	//console->PrintDebugMsg("> ", nullptr, MsgType::INLINE);
	//console->PrintDebugMsg("Reading %i ", (void*)numBytes, MsgType::INLINE);
	//console->PrintDebugMsg("bytes at %p", (void*)address, MsgType::INLINE);
	//console->NewLine();

	//TODO: Check if the address is bad somehow

	//DWORD oldProtection;
	//VirtualProtect((void*)address, numBytes, PAGE_EXECUTE_READ, &oldProtection);

	//std::vector<char> buffer = std::vector<char>(numBytes, '*');
	//memcpy(&buffer[0], (void*)address, numBytes);

	if (ReadProcessMemory(GetCurrentProcess(), address, buffer, numBytes, NULL) == 0)
	{
		console->PrintDebugMsg("> Memory read at: %p of", address, MsgType::INLINE);
		console->PrintDebugMsg(" %i bytes failed!", (void*)numBytes, MsgType::INLINE);
		console->NewLine();
		return false;
	}

	//VirtualProtect((void*)address, numBytes, oldProtection, &oldProtection);

	return true;
}