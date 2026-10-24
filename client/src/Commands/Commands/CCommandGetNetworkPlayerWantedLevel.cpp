#include "stdafx.h"
#include "CCommandGetNetworkPlayerWantedLevel.h"

void CCommandGetNetworkPlayerWantedLevel::Process(CRunningScript* script)
{
	script->CollectParameters(1);

	int pedHandle = ScriptParams[0];

	ScriptParams[0] = 0;

	if (auto ped = CPools::GetPed(pedHandle))
	{
		if (auto networkPlayer = CNetworkPlayerManager::GetPlayer(ped))
		{
			ScriptParams[0] = networkPlayer->m_nWantedLevel;
		}
	}

	script->StoreParameters(1);
}
