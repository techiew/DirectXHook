#pragma once

#include <string>
#include <vector>
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
	std::vector<std::string> msgQueue;
	int counter = 0;
	bool isDisabled = false;

public:

	DebugConsole();
	DebugConsole(std::string consoleName, bool showDebugConsole);
	void PrintDebugMsg(std::string msg, void* variable, MsgType msgType);
	void PrintSingleChar(char value, bool isHex);
};