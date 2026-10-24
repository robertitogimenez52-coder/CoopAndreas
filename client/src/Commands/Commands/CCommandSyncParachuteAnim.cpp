#include "stdafx.h"
#include "CCommandSyncParachuteAnim.h"
#include "CParachuteSync.h"

void CCommandSyncParachuteAnim::Process(CRunningScript* script)
{
	script->CollectParameters(2);

	// the PARACH script runs for any ped, only the local player's parachute is sent
	if (CPools::GetPed(ScriptParams[0]) == FindPlayerPed(0))
	{
		CParachuteSync::SendAnim(static_cast<uint8_t>(ScriptParams[1]));
	}
}
