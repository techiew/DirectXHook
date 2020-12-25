#include "DebugConsole.h"

bool DebugConsole::consoleOpen = false;
bool DebugConsole::globalMute = false;

DebugConsole::DebugConsole()
{
	muted = false;
}

// Mutes the current instance
void DebugConsole::Mute()
{
	muted = true;
}

void DebugConsole::Unmute()
{
	muted = false;
}

// Mutes every instance of DebugConsole
void DebugConsole::MuteAll()
{
	globalMute = true;
}

void DebugConsole::UnmuteAll()
{
	globalMute = false;
}

// Opens a console, only if one does not exist
void DebugConsole::Open()
{

	if (AllocConsole())
	{
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		SetWindowText(GetConsoleWindow(), "DebugConsole");
		consoleOpen = true;
	}

}

// Closes an existing console (only if DebugConsole owns it)
void DebugConsole::Close()
{
	if (!consoleOpen) return;

	PostMessage(GetConsoleWindow(), WM_CLOSE, 0, 0);
	FreeConsole();
}

void DebugConsole::Print()
{
	_Print(MsgType::PROGRESS, "", nullptr);
}

void DebugConsole::Print(std::string msg, ...)
{
	va_list args;
	va_start(args, msg);
	_Print(MsgType::PROGRESS, msg, args);
}

void DebugConsole::Print(MsgType msgType, std::string msg, ...)
{
	va_list args;
	va_start(args, msg);
	_Print(msgType, msg, args);
}

void DebugConsole::_Print(MsgType msgType, std::string msg, va_list args)
{
	if (muted || globalMute) return;

	switch (msgType)
	{
	case(STARTPROCESS):
		vprintf(std::string("+ " + msg + "\n").c_str(), args);
		break;
	case(PROGRESS):
		vprintf(std::string("  " + msg + "\n").c_str(), args);
		break;
	case(COMPLETE):
		vprintf(std::string("  " + msg + "\n").c_str(), args);
		break;
	case(FAILED): 
		vprintf(std::string("!! " + msg + " !!\n").c_str(), args);
		break;
	case(INLINE):
		vprintf(std::string(msg).c_str(), args);
		break;
	}

}