#include "DebugConsole.h"

bool DebugConsole::consoleOpen = false;

DebugConsole::DebugConsole()
{
	muted = false;
}

void DebugConsole::Mute()
{
	muted = true;
}

void DebugConsole::UnMute()
{
	muted = false;
}

void DebugConsole::Open()
{

	if (AllocConsole())
	{
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		SetWindowText(GetConsoleWindow(), "DebugConsole");
		consoleOpen = true;
	}

}

void DebugConsole::Close()
{
	if (!consoleOpen) return;

	PostMessage(GetConsoleWindow(), WM_CLOSE, 0, 0);
	FreeConsole();
}

void DebugConsole::Print(std::string msg)
{
	Print(msg, nullptr, MsgType::PROGRESS);
}

void DebugConsole::Print(std::string msg, void* value)
{
	Print(msg, value, MsgType::PROGRESS);
}

void DebugConsole::Print(std::string msg, MsgType msgType)
{
	Print(msg, nullptr, msgType);
}

void DebugConsole::Print(std::string msg, void* value, MsgType msgType)
{
	if (muted) return;

	switch (msgType)
	{
	case(STARTPROCESS):
		printf(std::string("+ " + msg + "\n").c_str(), value);
		break;
	case(PROGRESS):
		printf(std::string("  " + msg + "\n").c_str(), value);
		break;
	case(COMPLETE):
		printf(std::string("  " + msg + "\n").c_str(), value);
		break;
	case(FAILED): 
		printf(std::string("!! " + msg + " !!\n").c_str(), value);
		break;
	case(INLINE):
		printf(std::string(msg).c_str(), value);
		break;
	}

}

// I believe I did this because floats don't like to pretend to be void*
void DebugConsole::Print(std::string msg, float value)
{
	if (muted) return;
	printf(std::string("  " + msg + "\n").c_str(), value);
}

void DebugConsole::PrintHex(unsigned char hexValue)
{
	if (muted) return;
	printf("0x%X ", hexValue);
}