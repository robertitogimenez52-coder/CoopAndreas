#pragma once

#include <map>
#include <array>
#include <network/packets/world.h>

// the last gang territories sent by the host, for the players who join later
class CGangZoneState
{
public:
    using GangZonesUpdate = Packets::World::GangZonesUpdate;

    static inline bool ms_bGangWarsActive = false;
    static inline std::map<uint16_t, std::array<uint8_t, GangZonesUpdate::NUM_GANGS>> ms_zones{};

    static void Store(const GangZonesUpdate& packet)
    {
        ms_bGangWarsActive = packet.bGangWarsActive;
        for (int i = 0; i < packet.count; i++)
        {
            auto& zone = ms_zones[packet.zones[i].index];
            std::copy(std::begin(packet.zones[i].gangDensity), std::end(packet.zones[i].gangDensity), zone.begin());
        }
    }

    static void SendTo(CNetworkPlayer* pNetworkPlayer)
    {
        if (ms_zones.empty())
            return;

        GangZonesUpdate packet{};
        packet.bGangWarsActive = ms_bGangWarsActive;

        for (const auto& [index, densities] : ms_zones)
        {
            auto& zone = packet.zones[packet.count++];
            zone.index = index;
            std::copy(densities.begin(), densities.end(), std::begin(zone.gangDensity));

            if (packet.count == GangZonesUpdate::MAX_ZONES)
            {
                GetPacketFactory().Send(packet, pNetworkPlayer);
                packet.count = 0;
            }
        }

        if (packet.count)
        {
            GetPacketFactory().Send(packet, pNetworkPlayer);
        }
    }
};
