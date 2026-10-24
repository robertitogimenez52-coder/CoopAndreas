#pragma once

#include <network/packets/world.h>

// the toggled cheats, sent to the players who join later
class CCheatState
{
public:
    static inline bool ms_abActive[Packets::World::CheatSync::NUM_CHEATS]{};

    static void SendTo(CNetworkPlayer* pNetworkPlayer)
    {
        for (uint8_t i = 0; i < Packets::World::CheatSync::NUM_CHEATS; i++)
        {
            if (!ms_abActive[i])
                continue;

            Packets::World::CheatSync packet{};
            packet.cheatId = i;
            packet.bFunction = false;
            packet.bActive = true;
            GetPacketFactory().Send(packet, pNetworkPlayer);
        }
    }
};
