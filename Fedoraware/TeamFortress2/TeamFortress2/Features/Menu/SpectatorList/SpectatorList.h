#pragma once
#include "../../../SDK/SDK.h"

class CSpectatorList
{
private:
	struct Spectator_t
	{
		std::wstring Name;
		std::wstring Mode;
		float RespawnTime;
		bool IsFriend;
		int Team;
		int Index;
	};

	std::vector<Spectator_t> Spectators;
	int SpecListW = 300, SpecListTitleBarH = 12;

public:
	int SpecListX = 30, SpecListY = 100;
	bool GetSpectators(CBaseEntity* pLocal);
	bool ShouldRun();
	void Run();
	void DragSpecList(int& x, int& y, int w, int h, int offsety);
	void DrawDefault();
	void DrawClassic();
};

ADD_FEATURE(CSpectatorList, SpectatorList)