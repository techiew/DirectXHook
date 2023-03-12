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
#include "MemoryUtils.h"

class Timer
{
public:
	Timer(unsigned int millis)
	{
		interval = millis;
	}

	bool Check()
	{
		auto now = std::chrono::system_clock::now();
		if (resetOnNextCheck)
		{
			lastExecutionTime = now;
			resetOnNextCheck = false;
			return false;
		}

		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastExecutionTime);
		if (diff.count() >= interval)
		{
			lastExecutionTime = now;
			return true;
		}

		return false;
	}

	void Reset()
	{
		resetOnNextCheck = true;
	}
private:
	unsigned int interval = 0;
	bool resetOnNextCheck = true;
	std::chrono::system_clock::time_point lastExecutionTime;
};

class RiseDpsMeter : public IRenderCallback
{
public:
	void Setup();
	void Render();
	~RiseDpsMeter();

private:
	struct PlayerData
	{
		uint64_t totalDamage = 0;
		uint64_t previousTotalDamage = 0;
		uint64_t mostDamageInOneSecond = 0;
		double averageDps = 0.0;
		uintptr_t damageMemoryAddress = 0;
		std::vector<uintptr_t> damagePointerOffsets;
		bool damageAddressIsFound = false;
		Timer timerVerifyDamageAddress{ 10000 };
		std::vector<uint64_t> dpsHistory;
		void Reset()
		{
			totalDamage = 0;
			previousTotalDamage = 0;
			mostDamageInOneSecond = 0;
			averageDps = 0;
			dpsHistory.clear();
		}
	};
	std::vector<PlayerData> playerData;

	std::fstream dpsMeterConfigFile;
	std::string configFileName = "rise_dps_meter.cfg";
	OF::Box* cornerWindow = nullptr;
	OF::Box* dpsMeterWindow = nullptr;
	OF::Box* dpsMeterWindowDivider = nullptr;
	OF::Box* placeholderWindow = nullptr;
	OF::Box* placeholderOkButton = nullptr;
	OF::Box* placeholderOkButtonBorder = nullptr;
	std::vector<OF::Box*> graphColumns;
	Timer timerUpdateDps{ 1000 };
	Timer timerCornerText{ 3000 };
	bool showCornerText = true;
	bool showPlaceholder = false;
	bool showDpsMeter = false;
	bool userDisabledDpsMeter = false;
	int font = -1;

	void DrawDpsMeter();
	void DrawPlaceholder();
	void DrawCornerText();
	void UpdateDamageStats();
	void UpdateGraph();
	void ReadMemory();
	void ReadPlayerDamage(PlayerData* playerStats);
	void FindDamageMemoryAddress(PlayerData* playerStats);
	void SetPointerOffsets();
	void CheckHotkeys();
	void WriteToConfigFile();
	void ReadConfigFile(int* x, int* y);
	void ResetState();
};