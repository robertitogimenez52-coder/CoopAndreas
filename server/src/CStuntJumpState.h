#pragma once

#include <set>
#include <network/packets/world.h>

// the unique stunt jumps done by anyone, sent to the players who join later
class CStuntJumpState
{
public:
    static inline std::set<uint16_t> ms_doneJumps{};

    static void SendTo(CNetworkPlayer* pNetworkPlayer)
    {
        for (uint16_t jumpIndex : ms_doneJumps)
        {
            Packets::World::StuntJumpDone packet{};
            packet.jumpIndex = jumpIndex;
            packet.bReward = false;
            GetPacketFactory().Send(packet, pNetworkPlayer);
        }
    }
};
