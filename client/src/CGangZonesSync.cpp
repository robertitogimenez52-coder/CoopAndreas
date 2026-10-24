#include "stdafx.h"
#include "CGangZonesSync.h"
#include <CGangWars.h>
#include <CTheZones.h>
#include <CZoneInfo.h>

using GangZonesUpdate = Packets::World::GangZonesUpdate;

// CTheZones::ZoneInfoArray and CTheZones::TotalNumberOfZoneInfos, see gta-reversed
static auto& ms_aZoneInfos = *reinterpret_cast<CZoneInfo(*)[GangZonesUpdate::NUM_ZONE_INFOS]>(0xBA1DF0);
static auto& ms_nNumZoneInfos = *reinterpret_cast<int16_t*>(0xBA1DE8);

// the territories the others know about
static uint8_t ms_aaLastDensity[GangZonesUpdate::NUM_ZONE_INFOS][GangZonesUpdate::NUM_GANGS] = {};
static bool ms_bLastGangWarsActive = false;
static unsigned int ms_nLastCheckTime = 0;

static int GetNumZoneInfos()
{
    return std::clamp<int>(ms_nNumZoneInfos, 0, GangZonesUpdate::NUM_ZONE_INFOS);
}

// the gang wars are started and fought on the host only, the attackers are its network peds
static void CGangWars__Update_Hook()
{
    if (!CNetwork::m_bAuthenticated || CLocalPlayer::m_bIsHost)
    {
        CGangWars::Update();
    }
}

void CGangZonesSync::InjectHooks()
{
    patch::RedirectCall(0x53C122, CGangWars__Update_Hook);  // CGame::Process
}

void CGangZonesSync::Process()
{
    if (!CLocalPlayer::m_bIsHost)
        return;

    unsigned int nTime = GetTickCount();
    if (nTime < ms_nLastCheckTime + 1000)
        return;

    ms_nLastCheckTime = nTime;

    GangZonesUpdate packet{};
    packet.bGangWarsActive = CGangWars::bGangWarsActive;

    bool bSent = false;
    for (int i = 0; i < GetNumZoneInfos(); i++)
    {
        const auto& zoneInfo = ms_aZoneInfos[i];
        if (memcmp(ms_aaLastDensity[i], zoneInfo.m_nGangDensity, GangZonesUpdate::NUM_GANGS) == 0)
            continue;

        memcpy(ms_aaLastDensity[i], zoneInfo.m_nGangDensity, GangZonesUpdate::NUM_GANGS);

        auto& zone = packet.zones[packet.count++];
        zone.index = static_cast<uint16_t>(i);
        memcpy(zone.gangDensity, zoneInfo.m_nGangDensity, GangZonesUpdate::NUM_GANGS);

        if (packet.count == GangZonesUpdate::MAX_ZONES)
        {
            GetPacketFactory().Send(packet);
            packet.count = 0;
            bSent = true;
        }
    }

    if (packet.count || (!bSent && ms_bLastGangWarsActive != packet.bGangWarsActive))
    {
        GetPacketFactory().Send(packet);
    }

    ms_bLastGangWarsActive = packet.bGangWarsActive;
}

void CGangZonesSync::HandlePacket(GangZonesUpdate* pGangZonesUpdate)
{
    if (CLocalPlayer::m_bIsHost)
        return;

    for (int i = 0; i < pGangZonesUpdate->count; i++)
    {
        const auto& zone = pGangZonesUpdate->zones[i];
        if (zone.index >= GetNumZoneInfos())
            continue;

        memcpy(ms_aZoneInfos[zone.index].m_nGangDensity, zone.gangDensity, GangZonesUpdate::NUM_GANGS);
        memcpy(ms_aaLastDensity[zone.index], zone.gangDensity, GangZonesUpdate::NUM_GANGS);
    }

    CGangWars::bGangWarsActive = pGangZonesUpdate->bGangWarsActive;
    ms_bLastGangWarsActive = pGangZonesUpdate->bGangWarsActive;

    // the territories on the radar and the map
    CTheZones::FillZonesWithGangColours(!CGangWars::bGangWarsActive);
}
