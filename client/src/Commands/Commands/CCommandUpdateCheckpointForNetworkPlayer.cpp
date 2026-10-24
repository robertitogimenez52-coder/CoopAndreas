#include "stdafx.h"
#include "CCommandUpdateCheckpointForNetworkPlayer.h"
#include "CMissionMarkerSync.h"

void CCommandUpdateCheckpointForNetworkPlayer::Process(CRunningScript* script)
{
	script->CollectParameters(7);

	// the script places the network players' checkpoints, the host's ones aren't mirrored
	CMissionMarkerSync::ms_bScriptHandlesCheckpoints = true;
	
	//CChat::AddMessage("CCommandUpdateCheckpointForNetworkPlayer::Process");
	Packets::Blips::UpdateCheckpoint packet{};
	packet.forWhoPlayerId = CNetworkPlayerManager::GetPlayer(CPools::GetPed(ScriptParams[6]))->m_playerId;
	packet.position = *(CVector*)&ScriptParams[0];
	packet.radius = *(CVector*)&ScriptParams[3];
	GetPacketFactory().Send(packet);
}
