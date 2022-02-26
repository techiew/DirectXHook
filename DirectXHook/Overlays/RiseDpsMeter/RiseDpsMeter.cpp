#include "RiseDpsMeter.h"

using namespace OF;

void RiseDpsMeter::Setup()
{
	InitFramework(m_device, m_spriteBatch, m_window);

	int defaultXPos = ofWindowWidth / 2;
	int defaultYPos = ofWindowHeight / 2;
	ReadConfigFile(&defaultXPos, &defaultYPos);

	m_dpsMeterWindow = CreateBox(defaultXPos, defaultYPos, 400, 180);
	m_dpsMeterWindowDivider = CreateBox(m_dpsMeterWindow, 23, m_dpsMeterWindow->height - 40, m_dpsMeterWindow->width - 46, 1);
	m_placeholderWindow = CreateBox(defaultXPos, defaultYPos, m_dpsMeterWindow->width, m_dpsMeterWindow->height);
	m_placeholderOkButton = CreateBox(m_placeholderWindow, m_placeholderWindow->width / 2 - 30, m_placeholderWindow->height - 40, 60, 30);
	m_placeholderOkButtonBorder = CreateBox(m_placeholderWindow, m_placeholderWindow->width / 2 - 27, m_placeholderWindow->height - 17, 55, 2);
	m_cornerWindow = CreateBox(0, 0, 50, 50);

	m_font = LoadFont("hook_fonts\\OpenSans-22.spritefont");
	SetFont(m_font);

	int numColumns = m_dpsMeterWindowDivider->width - 4;
	for (int i = 0; i < numColumns; i++)
	{
		m_graphColumns.push_back(CreateBox(m_dpsMeterWindow, i + 25, m_dpsMeterWindow->height - 41, 1, 0));
	}
}

void RiseDpsMeter::Render()
{
	CheckMouseEvents();
	CheckHotkeys();

	m_playerOneTotalDamage = ReadPlayerOneDamage();
	if (m_playerOneTotalDamage == 0)
	{
		if (m_playerOnePreviousTotalDamage != 0)
		{
			ResetState();
		}
	}
	else
	{
		if (m_timerUpdateDps.Check())
		{
			UpdateDamageStats();
			UpdateGraph();
		}

		if (!m_userDisabledDpsMeter)
		{
			DrawDpsMeter();
		}
	}

	if (m_showPlaceholder)
	{
		DrawPlaceholder();
	}

	if (m_showCornerText)
	{
		if (m_timerCornerText.Check())
		{
			m_showCornerText = false;
		}
		DrawCornerText();
	}
}

void RiseDpsMeter::DrawDpsMeter()
{
	std::ostringstream playerOneAvgDpsString;
	playerOneAvgDpsString << std::fixed << std::setprecision(1) << m_playerOneAvgDps;

	DrawBox(m_dpsMeterWindow, 0, 0, 0, 130);
	DrawBox(m_dpsMeterWindowDivider, 255, 255, 255, 255);

	for (auto box : m_graphColumns)
	{
		DrawBox(box, 125, 125, 255, 255);
	}

	DrawText(m_dpsMeterWindow, "DPS: " + playerOneAvgDpsString.str(), 20, m_dpsMeterWindow->height - 32, 0.6f);
	DrawText(m_dpsMeterWindow, "High: " + std::to_string(m_playerOneMostDmgInOneSecond), 162, m_dpsMeterWindow->height - 32, 0.6f);
	DrawText(m_dpsMeterWindow, "Total: " + std::to_string(m_playerOneTotalDamage), 287, m_dpsMeterWindow->height - 32, 0.6f);
}

void RiseDpsMeter::DrawPlaceholder()
{
	DrawBox(m_placeholderWindow, 0, 0, 0, 170);
	DrawText(m_placeholderWindow,
		"          (Hold Left Alt and drag to move)\n"
		"Move me to a suitable location then click ok.\n              "
		"The window will auto-hide.",
		23, 45, 0.6f);

	int color = 170;
	if (m_placeholderOkButton->hover)
	{
		color = 255;
	}

	DrawBox(m_placeholderOkButton, 0, 0, 0, 0);
	DrawBox(m_placeholderOkButtonBorder, color, color, color, 255);
	DrawText(m_placeholderOkButton, "Ok", 14, -3, 0.7f, color, color, color);

	if (m_placeholderOkButton->clicked)
	{
		m_showPlaceholder = false;
		m_dpsMeterWindow->x = m_placeholderWindow->x;
		m_dpsMeterWindow->y = m_placeholderWindow->y;
		m_dpsMeterConfigFile.open(m_configFileName, std::fstream::out);
	}
}

void RiseDpsMeter::DrawCornerText()
{
	DrawText(m_cornerWindow, "RiseDpsMeter v1.1 loaded", 0, 0, 0.5f);
}

void RiseDpsMeter::UpdateDamageStats()
{
	if (m_dpsHistory.size() > 21600)
	{
		m_dpsHistory.erase(m_dpsHistory.begin());
	}

	m_dpsHistory.push_back(m_playerOneTotalDamage - m_playerOnePreviousTotalDamage);
	m_playerOnePreviousTotalDamage = m_playerOneTotalDamage;

	if (m_dpsHistory.back() > m_playerOneMostDmgInOneSecond)
	{
		m_playerOneMostDmgInOneSecond = m_dpsHistory.back();
	}

	m_playerOneAvgDps = 0;
	for (auto dps : m_dpsHistory)
	{
		m_playerOneAvgDps += dps;
	}
	m_playerOneAvgDps /= m_dpsHistory.size();
}

void RiseDpsMeter::UpdateGraph()
{
	for (int i = m_graphColumns.size() - 1, j = m_dpsHistory.size() - 1;
		i > -1 && j > -1;
		i--, j--)
	{
		if (m_dpsHistory[j] != 0)
		{
			m_graphColumns[i]->height = MapIntToRange(m_dpsHistory[j], 0, m_playerOneMostDmgInOneSecond, 0, 130);
			m_graphColumns[i]->y = m_dpsMeterWindow->height - 41 - m_graphColumns[i]->height;
		}

		if (i != m_graphColumns.size() - 1 && j != m_dpsHistory.size() - 1 && m_dpsHistory[j + 1] == 0)
		{
			m_graphColumns[i + 1]->height = m_graphColumns[i]->height * 0.8;
			m_graphColumns[i + 1]->y = m_dpsMeterWindow->height - 41 - m_graphColumns[i + 1]->height;
		}
	}
}

uint64_t RiseDpsMeter::ReadPlayerOneDamage()
{
	m_playerOneDamageAddress = ReadPointerChain(m_playerOneDamagePointerChain);
	if (m_playerOneDamageAddress != 0)
	{
		return *(uintptr_t*)m_playerOneDamageAddress;
	}
	return 0;
}

uintptr_t RiseDpsMeter::ReadPointerChain(std::vector<uintptr_t> pointerChain)
{
	uintptr_t pointer = pointerChain[0];
	MEMORY_BASIC_INFORMATION memoryInfo;
	for (int i = 0; i < pointerChain.size() - 1; i++)
	{
		if (!VirtualQuery((void*)pointer, &memoryInfo, sizeof(MEMORY_BASIC_INFORMATION)))
		{
			return 0;
		} 
		else if (memoryInfo.Protect == PAGE_READWRITE && memoryInfo.State == MEM_COMMIT)
		{
			pointer = *(uintptr_t*)pointer;
			if (pointer == 0)
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}

		pointer += pointerChain[i + 1];
	}
	return pointer;
}

void RiseDpsMeter::CheckHotkeys()
{
	if (CheckHotkey('P', VK_LSHIFT))
	{
		m_placeholderWindow->x = ofWindowWidth / 2;
		m_placeholderWindow->y = ofWindowHeight / 2;
		m_dpsMeterWindow->x = ofWindowWidth / 2;
		m_dpsMeterWindow->y = ofWindowHeight / 2;
	}
	else if (CheckHotkey('P'))
	{
		if (m_userDisabledDpsMeter)
		{
			m_userDisabledDpsMeter = false;
		}
		else
		{
			m_userDisabledDpsMeter = true;
		}
	} 

	if (CheckHotkey(' ', VK_LMENU))
	{
		m_dpsMeterWindow->draggable = true;
		m_placeholderWindow->draggable = true;
	}
	else
	{
		m_dpsMeterWindow->draggable = false;
		m_placeholderWindow->draggable = false;
	}
}

void RiseDpsMeter::ReadConfigFile(int* x, int* y)
{
	m_dpsMeterConfigFile.open(m_configFileName, std::fstream::in);

	if (m_dpsMeterConfigFile.is_open())
	{
		std::string line = "";
		getline(m_dpsMeterConfigFile, line);
		std::stringstream stringStream(line);
		std::istream_iterator<std::string> begin(stringStream);
		std::istream_iterator<std::string> end;
		std::vector<std::string> values(begin, end);
		if (values.size() == 2)
		{
			*x = std::stoi(values[0]);
			*y = std::stoi(values[1]);
		}
		m_dpsMeterConfigFile.close();
		m_dpsMeterConfigFile.open(m_configFileName, std::fstream::out);
	}
	else
	{
		m_showPlaceholder = true;
	}
}

void RiseDpsMeter::ResetState()
{
	m_dpsHistory.clear();
	for (auto box : m_graphColumns)
	{
		box->height = 0;
	}
	m_playerOneTotalDamage = 0;
	m_playerOnePreviousTotalDamage = 0;
	m_playerOneAvgDps = 0;
	m_playerOneMostDmgInOneSecond = 0;
	m_timerUpdateDps.Reset();
}

RiseDpsMeter::~RiseDpsMeter()
{
	if (m_dpsMeterConfigFile.is_open())
	{
		m_dpsMeterConfigFile << m_dpsMeterWindow->x << " " << m_dpsMeterWindow->y << std::endl;
		m_dpsMeterConfigFile.close();
	}
}