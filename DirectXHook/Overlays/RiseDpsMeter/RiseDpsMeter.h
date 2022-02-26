#pragma once

#include <SpriteBatch.h>
#include <d3d11.h>
#include <vector>
#include <iomanip>
#include <sstream>
#include <istream>
#include <iterator>

#include "IRenderCallback.h"
#include "OverlayFramework.h"

class Timer
{
public:
	Timer(unsigned int millis)
	{
		m_interval = millis;
	}

	bool Check()
	{
		auto now = std::chrono::system_clock::now();
		if (m_resetOnNextCheck)
		{
			m_lastExecutionTime = now;
			m_resetOnNextCheck = false;
			return false;
		}

		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastExecutionTime);
		if (diff.count() >= m_interval)
		{
			m_lastExecutionTime = now;
			return true;
		}

		return false;
	}

	void Reset()
	{
		m_resetOnNextCheck = true;
	}
private:
	unsigned int m_interval = 0;
	bool m_resetOnNextCheck = true;
	std::chrono::system_clock::time_point m_lastExecutionTime;
};

class RiseDpsMeter : public IRenderCallback
{
public:
	void Setup();
	void Render();
	~RiseDpsMeter();

private:
	std::fstream m_dpsMeterConfigFile;
	std::string m_configFileName = "rise_dps_meter.cfg";
	OF::Box* m_cornerWindow = nullptr;
	OF::Box* m_dpsMeterWindow = nullptr;
	OF::Box* m_dpsMeterWindowDivider = nullptr;
	OF::Box* m_placeholderWindow = nullptr;
	OF::Box* m_placeholderOkButton = nullptr;
	OF::Box* m_placeholderOkButtonBorder = nullptr;
	std::vector<OF::Box*> m_graphColumns;
	std::vector<uint64_t> m_dpsHistory;
	uint64_t m_playerOneTotalDamage = 0;
	uint64_t m_playerOnePreviousTotalDamage = 0;
	uint64_t m_playerOneMostDmgInOneSecond = 0;
	double m_playerOneAvgDps = 0;
	Timer m_timerUpdateDps{ 1000 };
	Timer m_timerCornerText{ 3000 };
	bool m_showCornerText = true;
	bool m_showPlaceholder = false;
	bool m_showDpsMeter = false;
	bool m_userDisabledDpsMeter = false;
	int m_font = -1;

	std::vector<uintptr_t> m_playerOneDamagePointerChain =
	{
		0x14C0A8A30,
		0x70,
		0x30,
		0xB0,
		0x4E0,
		0x18
	};
	uintptr_t m_playerOneDamageAddress = 0;

	void DrawDpsMeter();
	void DrawPlaceholder();
	void DrawCornerText();
	void UpdateDamageStats();
	void UpdateGraph();
	uint64_t ReadPlayerOneDamage();
	uintptr_t ReadPointerChain(std::vector<uintptr_t> pointerChain);
	void CheckHotkeys();
	void ReadConfigFile(int* x, int* y);
	void ResetState();
};