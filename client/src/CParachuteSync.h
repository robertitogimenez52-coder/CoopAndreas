#pragma once

// the parachute object of the PARACH script (the ped's anims are synced as script tasks)
class CParachuteSync
{
public:
    // called by the Coop.SyncParachuteAnim script command for the local player
    static void SendAnim(uint8_t anim);
    static void HandlePacket(Packets::Players::PlayerParachute* pPlayerParachute);
    static void Process();
    static void RemoveParachute(CNetworkPlayer* pNetworkPlayer);
};
