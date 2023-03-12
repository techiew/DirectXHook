#include "RiseDpsMeter.h"

using namespace OF;

void RiseDpsMeter::Setup()
{
	InitFramework(device, spriteBatch, window);

	int defaultXPos = ofWindowWidth / 2;
	int defaultYPos = ofWindowHeight / 2;
	ReadConfigFile(&defaultXPos, &defaultYPos);

	dpsMeterWindow = CreateBox(defaultXPos, defaultYPos, 400, 180);
	dpsMeterWindowDivider = CreateBox(dpsMeterWindow, 23, dpsMeterWindow->height - 40, dpsMeterWindow->width - 46, 1);
	placeholderWindow = CreateBox(defaultXPos, defaultYPos, dpsMeterWindow->width, dpsMeterWindow->height);
	placeholderOkButton = CreateBox(placeholderWindow, placeholderWindow->width / 2 - 30, placeholderWindow->height - 40, 60, 30);
	placeholderOkButtonBorder = CreateBox(placeholderWindow, placeholderWindow->width / 2 - 27, placeholderWindow->height - 17, 55, 2);
	cornerWindow = CreateBox(0, 0, 50, 50);

	int columnWidth = 1;
	int numColumns = floor(((float)dpsMeterWindowDivider->width) / columnWidth) - (columnWidth * 2);
	for (int i = 0; i < numColumns; i++)
	{
		graphColumns.push_back(CreateBox(dpsMeterWindow, i * columnWidth + columnWidth + 25, dpsMeterWindow->height - 41, columnWidth, 0));
	}

	font = LoadFont("hook_fonts\\OpenSans-22.spritefont");
	SetFont(font);

	SetPointerOffsets();
}

void RiseDpsMeter::Render()
{
	CheckMouseEvents();
	CheckHotkeys();
	ReadMemory();

	if (playerData[0].totalDamage != 0)
	{
		if (timerUpdateDps.Check())
		{
			UpdateDamageStats();
			UpdateGraph();
		}

		if (!userDisabledDpsMeter)
		{
			DrawDpsMeter();
		}
	}

	if (showPlaceholder)
	{
		DrawPlaceholder();
	}

	if (showCornerText)
	{
		if (timerCornerText.Check())
		{
			showCornerText = false;
		}
		DrawCornerText();
	}
}

void RiseDpsMeter::DrawDpsMeter()
{
	std::ostringstream dpsString;
	dpsString << std::fixed << std::setprecision(1) << playerData[0].averageDps;

	DrawBox(dpsMeterWindow, 0, 0, 0, 130);
	DrawBox(dpsMeterWindowDivider, 255, 255, 255, 255);

	for (auto box : graphColumns)
	{
		DrawBox(box, 125, 125, 255, 255);
	}

	DrawText(dpsMeterWindow, "DPS: " + dpsString.str(), 20, dpsMeterWindow->height - 32, 0.6f);
	DrawText(dpsMeterWindow, "High: " + std::to_string(playerData[0].mostDamageInOneSecond), 162, dpsMeterWindow->height - 32, 0.6f);
	DrawText(dpsMeterWindow, "Total: " + std::to_string(playerData[0].totalDamage), 287, dpsMeterWindow->height - 32, 0.6f);
}

void RiseDpsMeter::DrawPlaceholder()
{
	DrawBox(placeholderWindow, 0, 0, 0, 170);
	DrawText(placeholderWindow,
		"          (Hold Left Alt and drag to move)\n"
		"Move me to a suitable location then click ok.\n              "
		"The window will auto-hide.",
		23, 45, 0.6f);

	int color = 170;
	if (placeholderOkButton->hover)
	{
		color = 255;
	}

	DrawBox(placeholderOkButton, 0, 0, 0, 0);
	DrawBox(placeholderOkButtonBorder, color, color, color, 255);
	DrawText(placeholderOkButton, "Ok", 14, -3, 0.7f, color, color, color);

	if (placeholderOkButton->clicked)
	{
		showPlaceholder = false;
		dpsMeterWindow->x = placeholderWindow->x;
		dpsMeterWindow->y = placeholderWindow->y;
		WriteToConfigFile();
	}
}

void RiseDpsMeter::DrawCornerText()
{
	DrawText(cornerWindow, "RiseDpsMeter v2.0 loaded", 0, 0, 0.5f);
}

void RiseDpsMeter::UpdateDamageStats()
{
	for (auto& player : playerData)
	{
		if (player.dpsHistory.size() > 21600)
		{
			player.dpsHistory.erase(player.dpsHistory.begin());
		}

		player.dpsHistory.push_back(player.totalDamage - player.previousTotalDamage);
		player.previousTotalDamage = player.totalDamage;

		if (player.dpsHistory.back() > player.mostDamageInOneSecond)
		{
			player.mostDamageInOneSecond = player.dpsHistory.back();
		}

		player.averageDps = 0;
		for (auto dps : player.dpsHistory)
		{
			player.averageDps += dps;
		}
		player.averageDps /= player.dpsHistory.size();
	}
}

void RiseDpsMeter::UpdateGraph()
{
	for (int i = graphColumns.size() - 1, 
		j = playerData[0].dpsHistory.size() - 1;
		i > -1 && j > -1;
		i--, 
		j--)
	{
		if (playerData[0].dpsHistory[j] != 0)
		{
			graphColumns[i]->height = MapIntToRange(playerData[0].dpsHistory[j], 0, playerData[0].mostDamageInOneSecond, 0, 130);
			graphColumns[i]->y = dpsMeterWindow->height - 41 - graphColumns[i]->height;
		}

		if (i != graphColumns.size() - 1 && j != playerData[0].dpsHistory.size() - 1 && playerData[0].dpsHistory[j + 1] == 0)
		{
			graphColumns[i + 1]->height = graphColumns[i]->height * 0.8;
			graphColumns[i + 1]->y = dpsMeterWindow->height - 41 - graphColumns[i + 1]->height;
		}
	}
}

void RiseDpsMeter::ReadMemory()
{
	for (auto& player : playerData)
	{
		ReadPlayerDamage(&player);
	}
}

void RiseDpsMeter::ReadPlayerDamage(PlayerData* playerData)
{
	if (playerData->timerVerifyDamageAddress.Check())
	{
		FindDamageMemoryAddress(playerData);
	}

	if (playerData->damageAddressIsFound)
	{
		MemoryUtils::MemCopy(
			(uintptr_t)&playerData->totalDamage,
			playerData->damageMemoryAddress,
			sizeof(uint64_t));
	}
	else
	{
		ResetState();
	}
}

void RiseDpsMeter::FindDamageMemoryAddress(PlayerData* playerData)
{
	if (playerData->damagePointerOffsets.size() > 0)
	{
		playerData->damageMemoryAddress = MemoryUtils::ReadPointerChain(playerData->damagePointerOffsets);
	}

	if (playerData->damageMemoryAddress == 0)
	{
		playerData->damageAddressIsFound = false;
	}
	else
	{
		playerData->damageAddressIsFound = true;
	}
}

void RiseDpsMeter::SetPointerOffsets()
{
	playerData.resize(1);
	playerData[0].damagePointerOffsets = {
		0x0f6e7550,
		0xd8,
		0x110,
		0x20,
		0x20,
		0x138,
		0x5c8,
		0x18
	};
}

void RiseDpsMeter::CheckHotkeys()
{
	if (CheckHotkey('P', VK_LSHIFT))
	{
		placeholderWindow->x = ofWindowWidth / 2;
		placeholderWindow->y = ofWindowHeight / 2;
		dpsMeterWindow->x = ofWindowWidth / 2;
		dpsMeterWindow->y = ofWindowHeight / 2;
	}
	else if (CheckHotkey('P'))
	{
		if (userDisabledDpsMeter)
		{
			userDisabledDpsMeter = false;
		}
		else
		{
			userDisabledDpsMeter = true;
		}
	} 

	if (CheckHotkey(UNBOUND, VK_LMENU))
	{
		dpsMeterWindow->draggable = true;
		placeholderWindow->draggable = true;
	}
	else
	{
		dpsMeterWindow->draggable = false;
		placeholderWindow->draggable = false;
	}
}

void RiseDpsMeter::WriteToConfigFile()
{
	dpsMeterConfigFile.open(configFileName, std::fstream::out);
	if (dpsMeterConfigFile.is_open() && dpsMeterWindow != nullptr)
	{
		dpsMeterConfigFile << dpsMeterWindow->x << " " << dpsMeterWindow->y << std::endl;
		dpsMeterConfigFile.close();
	}
}

void RiseDpsMeter::ReadConfigFile(int* x, int* y)
{
	dpsMeterConfigFile.open(configFileName, std::fstream::in);
	if (dpsMeterConfigFile.is_open())
	{
		std::string line = "";
		getline(dpsMeterConfigFile, line);
		std::stringstream stringStream(line);
		std::istream_iterator<std::string> begin(stringStream);
		std::istream_iterator<std::string> end;
		std::vector<std::string> values(begin, end);
		if (values.size() >= 2)
		{
			*x = std::stoi(values[0]);
			*y = std::stoi(values[1]);
		}
		dpsMeterConfigFile.close();
	}
	else
	{
		showPlaceholder = true;
	}
}

void RiseDpsMeter::ResetState()
{
	for (auto box : graphColumns)
	{
		box->height = 0;
	}
	for (auto& player : playerData)
	{
		player.Reset();
	}
	timerUpdateDps.Reset();
}

RiseDpsMeter::~RiseDpsMeter()
{
	WriteToConfigFile();
}