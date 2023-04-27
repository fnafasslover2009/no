#include "CritHack.h"
#define MASK_SIGNED 0x7FFFFFFF

// i hate crithack

/* Returns whether random crits are enabled on the server */
bool CCritHack::AreRandomCritsEnabled()
{
	if (static auto tf_weapon_criticals = g_ConVars.FindVar("tf_weapon_criticals"); tf_weapon_criticals)
	{
		return tf_weapon_criticals->GetBool();
	}
	return true;
}

/* Returns whether the crithack should run */
bool CCritHack::IsEnabled()
{
	if (!Vars::CritHack::Active.Value) { return false; }
	if (!AreRandomCritsEnabled()) { return false; }
	if (!I::EngineClient->IsInGame()) { return false; }

	return true;
}

bool CCritHack::IsAttacking(const CUserCmd* pCmd, CBaseCombatWeapon* pWeapon)
{
	if (G::CurItemDefIndex == Soldier_m_TheBeggarsBazooka)
	{
		static bool bLoading = false;

		if (pWeapon->GetClip1() > 0)
		{
			bLoading = true;
		}

		if (!(pCmd->buttons & IN_ATTACK) && bLoading)
		{
			bLoading = false;
			return true;
		}
	}

	else
	{
		if (pWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW || pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER)
		{
			static bool bCharging = false;

			if (pWeapon->GetChargeBeginTime() > 0.0f)
			{
				bCharging = true;
			}

			if (!(pCmd->buttons & IN_ATTACK) && bCharging)
			{
				bCharging = false;
				return true;
			}
		}

		//pssst..
		//Dragon's Fury has a gauge (seen on the weapon model) maybe it would help for pSilent hmm..
		/*
		if (pWeapon->GetWeaponID() == 109) {
		}*/

		else
		{
			if ((pCmd->buttons & IN_ATTACK) && G::WeaponCanAttack)
			{
				return true;
			}
		}
	}

	return false;
}

bool CCritHack::NoRandomCrits(CBaseCombatWeapon* pWeapon)
{
	float CritChance = Utils::ATTRIB_HOOK_FLOAT(1, "mult_crit_chance", pWeapon);
	if (CritChance == 0)
	{
		return true;
	}
	else 
	return false;
	//list of weapons that cant random crit, but dont have the attribute for it
	switch (pWeapon->GetWeaponID())
	{
		//scout
		case TF_WEAPON_JAR_MILK:
		//soldier
		case TF_WEAPON_BUFF_ITEM:
		//pyro
		case TF_WEAPON_JAR_GAS:
		case TF_WEAPON_FLAME_BALL:
		case TF_WEAPON_ROCKETPACK:
		//demo
		case TF_WEAPON_PARACHUTE: //also for soldier
		//heavy
		case TF_WEAPON_LUNCHBOX:
		//engineer
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_LASER_POINTER:
		//medic
		case TF_WEAPON_MEDIGUN:
		//sniper
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_COMPOUND_BOW:
		case TF_WEAPON_JAR:
		//spy
		case TF_WEAPON_KNIFE:
		case TF_WEAPON_PDA_SPY_BUILD:
		case TF_WEAPON_PDA_SPY:
			return true;
			break;
		default: return false; break;
	}
}

bool CCritHack::ShouldCrit()
{
	static KeyHelper critKey{ &Vars::CritHack::CritKey.Value };
	if (critKey.Down()) { return true; }
	if (G::CurWeaponType == EWeaponType::MELEE && Vars::CritHack::AlwaysMelee.Value) { return true; }

	return false;
}

int CCritHack::LastGoodCritTick(const CUserCmd* pCmd)
{
	int lastCritTick = -1;
	bool shouldPopBack = false;

	for (auto nigga = CritTicks.rbegin(); nigga != CritTicks.rend(); ++nigga)
	{
		if (*nigga >= pCmd->command_number)
		{
			lastCritTick = *nigga;
		}
		else
		{
			shouldPopBack = true;
		}
	}

	if (shouldPopBack)
	{
		CritTicks.erase(CritTicks.begin(), std::find(CritTicks.begin(), CritTicks.end(), lastCritTick));
	}

	if (auto netChan = I::EngineClient->GetNetChannelInfo())
	{
		const int lastOutSeqNr = netChan->m_nOutSequenceNr;
		const int newOutSeqNr = pCmd->command_number - 1;
		if (newOutSeqNr > lastOutSeqNr)
		{
			netChan->m_nOutSequenceNr = newOutSeqNr;
		}
	}

	if (auto localPlayer = g_EntityCache.GetLocal())
	{
		if (auto activeWeapon = localPlayer->GetActiveWeapon())
		{
			if (activeWeapon->WillCrit())
			{
				CritTicks.push_back(pCmd->command_number);
			}
		}
	}

	return lastCritTick;
}


void CCritHack::ScanForCrits(const CUserCmd* pCmd, int loops)
{
	static int previousWeapon = 0;
	static int startingNum = pCmd->command_number - 1;
	const auto& pLocal = g_EntityCache.GetLocal();
	if (!pLocal || G::IsAttacking) {
		return;
	}
	const auto& pWeapon = pLocal->GetActiveWeapon();
	if (!pWeapon || IsAttacking(pCmd, pWeapon)) {
		return;
	}
	const bool bRescanRequired = previousWeapon != pWeapon->GetIndex();
	if (bRescanRequired) {
		startingNum = pCmd->command_number;
		previousWeapon = pWeapon->GetIndex();
		CritTicks.clear();
	}
	if (CritTicks.size() >= 256) {
		return;
	}
	int seedBackup = MD5_PseudoRandom(pCmd->command_number) & MASK_SIGNED;
	for (int i = 0; i < loops; i++) {
		const int cmdNum = startingNum + i;
		*I::RandomSeed = MD5_PseudoRandom(cmdNum) & MASK_SIGNED;
		if (pWeapon->WillCrit()) {
			CritTicks.push_back(cmdNum);
		}
	}
	startingNum += loops;
	*I::RandomSeed = seedBackup;
	*reinterpret_cast<int*>(pWeapon + 0xA5C) = 1;
}  //i broke the crithack

void CCritHack::Run(CUserCmd* pCmd)
{
	if (!IsEnabled()) { return; }

	const auto& pWeapon = g_EntityCache.GetWeapon();
	if (!pWeapon || !pWeapon->CanFireCriticalShot(false)) { return; }

	ScanForCrits(pCmd, 50); // fill our vector slowly.

	const int closestGoodTick = LastGoodCritTick(pCmd); // retrieve our wish
	if (IsAttacking(pCmd, pWeapon)) // is it valid & should we even use it
	{
		if (ShouldCrit() && closestGoodTick >= 0) // only crit if we have a good tick
		{
			pCmd->command_number = closestGoodTick;
			pCmd->random_seed = MD5_PseudoRandom(closestGoodTick) & MASK_SIGNED;
		}
		else if (Vars::CritHack::AvoidRandom.Value) // avoid random crits if enabled
		{
			// We will only increment the command number if it doesn't land on a tick
			// where we previously found a random crit.
			const int maxTries = 25;
			int tries = 0;
			while (tries < maxTries)
			{
				tries++;
				const int newCmdNum = pCmd->command_number + tries;
				if (std::find(CritTicks.begin(), CritTicks.end(), newCmdNum) != CritTicks.end())
				{
					continue;
				}
				pCmd->command_number = newCmdNum;
				pCmd->random_seed = MD5_PseudoRandom(newCmdNum) & MASK_SIGNED;
				CritTicks.push_back(newCmdNum);
				break;
			}
		}
	}
}

void CCritHack::Draw()
{
	if (!Vars::CritHack::Indicators.Value) { return; }
	if (!IsEnabled() || !G::CurrentUserCmd) { return; }

	const auto& pLocal = g_EntityCache.GetLocal();
	if (!pLocal || !pLocal->IsAlive()) { return; }

	const auto& pWeapon = pLocal->GetActiveWeapon();
	if (!pWeapon) { return; }

	const int x = Vars::CritHack::IndicatorPos.c;
	int currentY = Vars::CritHack::IndicatorPos.y;

	const float bucket = *reinterpret_cast<float*>(pWeapon + 0xA54);
	const int seedRequests = *reinterpret_cast<int*>(pWeapon + 0xA5C);

	int longestW = 40;

	if (Vars::Debug::DebugInfo.Value)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 255, 255, 255, 255, }, ALIGN_CENTERHORIZONTAL, tfm::format("%x", reinterpret_cast<float*>(pWeapon + 0xA54)).c_str());
	}
	// Are we currently forcing crits?
	if (ShouldCrit() && NoRandomCrits(pWeapon) == false)
	{
		if (CritTicks.size() > 0)
		{ 
			g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 0, 255, 0, 255 }, ALIGN_CENTERHORIZONTAL, "Forcing Crits");
		}
	}
	//Can this weapon do random crits?
	if (NoRandomCrits(pWeapon) == true)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 255, 95, 95, 255 }, ALIGN_CENTERHORIZONTAL, L"No Random Crits");
	}
	//Crit banned check
	if (CritTicks.size() == 0 && NoRandomCrits(pWeapon) == false)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 255,0,0,255 }, ALIGN_CENTERHORIZONTAL, L"Crit Banned");
	}
	static auto tf_weapon_criticals_bucket_cap = g_ConVars.FindVar("tf_weapon_criticals_bucket_cap");
	const float bucketCap = tf_weapon_criticals_bucket_cap->GetFloat();
	const std::wstring bucketstr = L"Bucket: " + std::to_wstring(static_cast<int>(bucket)) + L"/" + std::to_wstring(static_cast<int>(bucketCap));
	// crit bucket string (this sucks)
	if (NoRandomCrits(pWeapon) == false)
	{
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, bucketstr.c_str());
	}
	int w, h;
	I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, bucketstr.c_str(), w, h);
	if (w > longestW)
	{
		longestW = w;
	}
	if (Vars::Debug::DebugInfo.Value)
	{
		const std::wstring seedText = L"m_nCritSeedRequests: " + std::to_wstring(seedRequests);
		const std::wstring FoundCrits = L"Found Crit Ticks: " + std::to_wstring(CritTicks.size());
		const std::wstring commandNumber = L"cmdNumber: " + std::to_wstring(G::CurrentUserCmd->command_number);
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, seedText.c_str());
		I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, seedText.c_str(), w, h);
		if (w > longestW)
		{
			longestW = w;
		}
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, FoundCrits.c_str());
		I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, FoundCrits.c_str(), w, h);
		if (w > longestW)
		{
			longestW = w;
		}
		g_Draw.String(FONT_INDICATORS, x, currentY += 15, { 181, 181, 181, 255 }, ALIGN_CENTERHORIZONTAL, commandNumber.c_str());
		I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts.at(FONT_INDICATORS).dwFont, commandNumber.c_str(), w, h);
		if (w > longestW)
		{
			longestW = w;
		}
	}
	IndicatorW = longestW * 2;
	IndicatorH = currentY;
}
