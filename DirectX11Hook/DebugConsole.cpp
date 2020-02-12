#include "DebugConsole.h"

DebugConsole::DebugConsole(std::string consoleName, bool showDebugConsole)
{

	if (!showDebugConsole)
	{
		isDisabled = true;
		return;
	}

	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	SetWindowText(GetConsoleWindow(), consoleName.c_str());
}

void DebugConsole::PrintDebugMsg(std::string msg)
{
	if (isDisabled) return;

	printf(std::string("> " + msg + "\n").c_str());
}

void DebugConsole::PrintDebugMsg(std::string msg, void* value)
{
	if (isDisabled) return;

	printf(std::string("> " + msg + "\n").c_str(), value);
}

void DebugConsole::PrintDebugMsg(std::string msg, void* value, MsgType msgType)
{
	if (isDisabled) return;

	switch (msgType)
	{
	case(STARTPROCESS):
		printf(std::string(" [+] " + msg + "\n").c_str(), value);
		break;
	case(PROGRESS):
		printf(std::string("> " + msg + "\n").c_str(), value);
		break;
	case(COMPLETE):
		printf(std::string("> " + msg + "\n").c_str(), value);
		break;
	case(FAILED): 
		printf(std::string(" [!] " + msg + "\n").c_str(), value);
		break;
	}

}

void DebugConsole::PrintDebugMsg(std::string msg, float value)
{
	if (isDisabled) return;

	printf(std::string("> " + msg + "\n").c_str(), value);
}

void DebugConsole::PrintHex(unsigned char hexValue)
{
	if (isDisabled) return;

	printf("0x%X ", hexValue);
}

void DebugConsole::NewLine()
{
	if (isDisabled) return;

	printf("\n");
}