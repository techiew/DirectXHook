#pragma once

#include <string>
#include <iostream>
#include <Windows.h>

enum MsgType
{
	STARTPROCESS,
	PROGRESS,
	COMPLETE,
	FAILED
};

class DebugConsole
{
private:
	bool isDisabled = false;

public:
	DebugConsole() {};
	DebugConsole(std::string consoleName, bool showDebugConsole);
	void PrintDebugMsg(std::string msg);
	void PrintDebugMsg(std::string msg, void* variable);
	void PrintDebugMsg(std::string msg, void* variable, MsgType msgType);
	void PrintHex(unsigned char hexValue);
	void NewLine();
};