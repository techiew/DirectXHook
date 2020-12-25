#pragma once

#include <string>
#include <iostream>
#include <Windows.h>
#include <cstdarg>

enum MsgType
{
	STARTPROCESS,
	PROGRESS,
	COMPLETE,
	FAILED,
	INLINE
};

class DebugConsole
{
public:
	DebugConsole();
	void Mute();
	void Unmute();
	static void MuteAll();
	static void UnmuteAll();
	void Open();
	void Close();
	void Print();
	void Print(std::string msg, ...);
	void Print(MsgType msgType, std::string msg, ...);

private:
	bool muted;
	static bool consoleOpen;
	static bool globalMute;

	void _Print(MsgType msgType, std::string msg, va_list args);
};