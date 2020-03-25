#include "Overlay.h"

// TBC: move position, size, etc to attributes.h???

struct Player
{
	std::string name = std::string(20, '\0');
	int damageTotal = 0.0f;
	float dps = 0.0f;
};

struct Session
{
	Player players[4];
	int durationMinutes = 0;
};

struct Sessions
{
	Session current;
	Session previous;
};

struct MemoryPaths
{
	long long* anchor = nullptr;
	long long anchorOffset = 0x260;
	long long offlineModeOffset = 0x38;

	long long* offlineModeName = nullptr;

	long long offlineModeNameOffsets[5] =
	{
		0x58,
		0x8,
		0x8,
		0x10,
		0x78
	};

	//long long p1NameOffsets[5] =
	//{
	//	0x260,
	//	0x38,
	//	0x508,
	//	0x78,
	//	0x749
	//};

	long long p1NameOffset = 0x53319;
	long long p2NameOffset = 0x1C0;
	long long p3NameOffset = 0x1C0;
	long long p4NameOffset = 0x1C0;

	long long p1DamageOffset = -0x6581;
	long long p2DamageOffset = -0x64A1;
	long long p3DamageOffset = -0x63C1;
	long long p4DamageOffset = -0x62E1;

	long long offlineNameOffset = -0x83FB0;
	long long offlineDamageOffset = 0x4CEE8;
};

MemoryPaths mp;
Sessions sessions;
bool inOfflineMode = false;
int maxNameLength = 20;

bool nospam = true;
int counter = 0;

void Overlay::ReadValues()
{
	long long* pointerBuffer = nullptr;
	long long* anchorAddress = nullptr;

	// Check if the anchor address is readable and save its stored pointer in pointerBuffer.
	if (mem.ReadMemory(mp.anchor, &pointerBuffer, sizeof(long long*)))
	{
		console->PrintDebugMsg("Pointer OK: %p", pointerBuffer);
	}
	else
	{
		return;
	}

	// Check if the address pointed to by the previously retrieved anchor pointer is 
	// readable and save the value that is stored at this address into pointerBuffer. 
	// The stored value is an address, all the memory addresses we want to read 
	// are offset relative from that address.
	if (mem.ReadMemory((void*)((long long)pointerBuffer + mp.anchorOffset), &anchorAddress, sizeof(long long*)))
	{
		console->PrintDebugMsg("Anchor address: %p", anchorAddress);
	}
	else
	{
		return;
	}

	// Check if we are in offline mode by checking if this specific pointer chain is valid
	if (mem.ReadMemory((void*)((long long)anchorAddress + mp.offlineModeOffset), &pointerBuffer, sizeof(long long*)))
	{
		console->PrintDebugMsg("Pointer OK: %p", pointerBuffer);

		if (mem.ReadMemory((void*)((long long)pointerBuffer), &pointerBuffer, sizeof(long long*)))
		{
			inOfflineMode = false;
		} 
		else
		{
			inOfflineMode = true;
		}

	}

	// Read name and damage value for the local player when in offline mode
	if (inOfflineMode)
	{
		console->PrintDebugMsg("Offline mode is enabled.");

		if (mp.offlineModeName == nullptr)
		{
			long long* address = (long long*)scanner.FindPattern("MonsterHunterWorld.exe", "\x80\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xA0", "xx????xxxxxxxxxxxxxxxxx");

			console->PrintDebugMsg("Address is: %p", address);

			if (address != 0)
			{
				mp.offlineModeName = address;
				mp.offlineModeName = (long long*)((long long)mp.offlineModeName + 0x16);
				console->PrintDebugMsg("mp.offlineModeName is: %p", mp.offlineModeName);
			}

		}

		if (mp.offlineModeName == nullptr) return;

		if (mem.ReadMemory(mp.offlineModeName, &pointerBuffer, sizeof(long long*)))
		{
			console->PrintDebugMsg("Pointer OK: %p", pointerBuffer);
		}
		else
		{
			return;
		}

		int localPlayerIndex = 0;
		std::string localPlayerName = std::string(20, '\0');
		int localPlayerDamage = 0;
		int successes = 0;

		for (int i = 0; i < 4; i++)
		{

			if (mem.ReadMemory((void*)((long long)pointerBuffer + mp.offlineModeNameOffsets[i]), &pointerBuffer, sizeof(long long*)))
			{
				console->PrintDebugMsg("Pointer OK: %p", pointerBuffer);
				successes++;
			}
			else
			{
				return;
			}

		}

		console->PrintDebugMsg("Successes: %i", (void*)successes);

		if (successes != 4) return;

		if (mem.ReadMemory((void*)((long long)pointerBuffer + mp.offlineModeNameOffsets[4]), &localPlayerName[0], maxNameLength))
		{

			//console->PrintDebugMsg("Pointer OK: %s", (void*)(std::string*)&localPlayerName);

			for (int i = 0; i < 4; i++)
			{

				if (sessions.current.players[i].name == localPlayerName)
				{
					localPlayerIndex = i;
					break;
				}

			}

		}

		mem.ReadMemory((void*)((long long)anchorAddress + mp.offlineDamageOffset), &localPlayerDamage, sizeof(int));

		sessions.current.players[localPlayerIndex].name = localPlayerName;
		sessions.current.players[localPlayerIndex].damageTotal = localPlayerDamage;

		console->PrintDebugMsg("Player name: %s", (void*)localPlayerName.c_str());
		console->PrintDebugMsg("Player damage: %i", (void*)localPlayerDamage);

		return;
	}

	//std::vector<char> buffer = std::vector<char>(21, '\0');
	long long* namePointers[4] = 
	{
		(long long*)((long long)anchorAddress + mp.p1NameOffset),
		(long long*)((long long)namePointers[0] + mp.p2NameOffset),
		(long long*)((long long)namePointers[1] + mp.p3NameOffset),
		(long long*)((long long)namePointers[2] + mp.p4NameOffset)
	};

	for (int i = 0; i < 4; i++)
	{

		if (mem.ReadMemory((void*)namePointers[i], &sessions.current.players[i].name, maxNameLength))
		{
			console->PrintDebugMsg("Pointer OK: %p", namePointers[i]);
			console->PrintDebugMsg("Player %i name: ", (void*)(i + 1), MsgType::INLINE);
			console->PrintDebugMsg("%s", (void*)(std::string*)&sessions.current.players[i].name);
		}

	}

	long long* damagePointers[4] = 
	{
		(long long*)((long long)namePointers[0] + mp.p1DamageOffset),
		(long long*)((long long)namePointers[1] + mp.p2DamageOffset),
		(long long*)((long long)namePointers[2] + mp.p3DamageOffset),
		(long long*)((long long)namePointers[3] + mp.p4DamageOffset)
	};

	for (int i = 0; i < 4; i++)
	{

		if (mem.ReadMemory((void*)damagePointers[i], &sessions.current.players[i].damageTotal, sizeof(int)))
		{
			console->PrintDebugMsg("Pointer OK: %p", damagePointers[i]);
			console->PrintDebugMsg("Player %i damage: ", (void*)(i + 1), MsgType::INLINE);
			console->PrintDebugMsg("%i", (void*)sessions.current.players[i].damageTotal);
		}

	}

}

void Overlay::Load()
{
	mp.anchor = (long long*)scanner.FindPattern("MonsterHunterWorld.exe", "\x45\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x00\x00\x00\xE0\xFF\x07\x45\x01\x00", "xxxxxxxxxxxxxx????xxxxxxxxx");
	mp.anchor = (long long*)((long long)mp.anchor + 0x25);

	console->PrintDebugMsg("mp.anchor is: %p", mp.anchor);

	font.arial = fonts.LoadFont(".\\hook_fonts\\arial_22.spritefont");;

	Size(MOUSE, Vec2(20, 20));
	PosSize(WINDOW_MAIN, Vec3(NDC(0.0, 0.0).x, NDC(0.0, 0.0).y, 1.0f), Vec2(550, 340));
	PosSize(TEXT_DPS, Vec3(250, 120, 1.0f), Vec2(0.8, 0.8));

	Behavior(MOUSE)->clickthrough = true;

	Parent(TEXT_DPS, WINDOW_MAIN);
	Color(WINDOW_MAIN, 22, 14, 27, 200);
	Text(TEXT_DPS, "Hello, World!");
	Font(TEXT_DPS, font.arial);
}

void Overlay::Draw()
{
	ReadValues();

	Pos(MOUSE, XMFLOAT3(relativeClickPos.x + Pos((ID)clickingID).x + Pos(uiParent[clickingID]).x, relativeClickPos.y + Pos((ID)clickingID).y + Pos(uiParent[clickingID]).y, 1.0f));
	DoBox(MOUSE);

	DoBox(WINDOW_MAIN);

	std::vector<char> buffer = std::vector<char>(sizeof(int), '*');
	mem.ReadMemory(&counter, &buffer[0], sizeof(int));
	int number = *(int*)&buffer[0];

	Text(TEXT_DPS, std::to_string(number));
	DoText(TEXT_DPS);

	if (GetAsyncKeyState(VK_F4))
	{

		if (nospam)
		{

			console->PrintDebugMsg("Depth values: ");

			for (int i = 0; i < uiPos.size(); i++)
			{
				console->PrintDebugMsg("Element %i:", (void*)i);
				console->PrintDebugMsg("Depth: %f", uiPos[i].z);
			}

		}

		nospam = false;
	}
	else
	{
		nospam = true;
	}

	counter++;

}