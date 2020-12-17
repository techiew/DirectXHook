#pragma once

#include <string>
#include <iostream>
#include <Windows.h>

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
	void UnMute();
	void Open();
	void Close();
	void Print(std::string msg);
	void Print(std::string msg, void* value);
	void Print(std::string msg, MsgType msgType);
	void Print(std::string msg, void* value, MsgType msgType);
	void Print(std::string msg, float value);
	void PrintHex(unsigned char hexValue);

private:
	bool muted;
	static bool consoleOpen;
};