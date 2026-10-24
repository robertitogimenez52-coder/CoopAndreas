#include "stdafx.h"
#include "CMissionMarkerSync.h"
#include <COpCodeSync.h>
#include <CShadows.h>

using MissionMarkers = Packets::Blips::MissionMarkers;

static constexpr float MAP_Z_LOW_LIMIT = -100.0f;

// the areas highlighted by the host's synced scripts this frame
static MissionMarkers::SMarker ms_aFrameMarkers[MissionMarkers::MAX_MARKERS];
static int ms_nFrameMarkers = 0;

// the last sent ones
static MissionMarkers::SMarker ms_aSentMarkers[MissionMarkers::MAX_MARKERS];
static int ms_nSentMarkers = 0;
static unsigned int ms_nLastSendTime = 0;

// the received ones, drawn until they're not refreshed for a while
static MissionMarkers::SMarker ms_aReceivedMarkers[MissionMarkers::MAX_MARKERS];
static int ms_nReceivedMarkers = 0;
static unsigned int ms_nReceiveTime = 0;

static void RenderArea(uint32_t id, const CVector& center, float halfSizeX, float halfSizeY)
{
    CVector pos = center;
    CShadows::RenderIndicatorShadow(id, SHADOW_ADDITIVE, nullptr, &pos, halfSizeX, 0.0f, 0.0f, -halfSizeY, 0);
}

// 0x485E00, the same as the original + collecting the areas of the host's synced scripts
static void CTheScripts__HighlightImportantArea_Hook(uint32_t id, float x1, float y1, float x2, float y2, float z)
{
    CVector2D inf(std::min(x1, x2), std::min(y1, y2));
    CVector2D sup(std::max(x1, x2), std::max(y1, y2));

    CVector center;
    center.x = (inf.x + sup.x) / 2.0f;
    center.y = (inf.y + sup.y) / 2.0f;
    center.z = (z <= MAP_Z_LOW_LIMIT) ? CWorld::FindGroundZForCoord(center.x, center.y) : z;

    float halfSizeX = sup.x - center.x;
    float halfSizeY = sup.y - center.y;

    RenderArea(id, center, halfSizeX, halfSizeY);

    if (CNetwork::m_bAuthenticated && CLocalPlayer::m_bIsHost && !CMissionMarkerSync::ms_bScriptHandlesCheckpoints &&
        ms_nFrameMarkers < MissionMarkers::MAX_MARKERS && COpCodeSync::IsScriptSynced(COpCodeSync::GetLastProcessedScript()))
    {
        auto& marker = ms_aFrameMarkers[ms_nFrameMarkers++];
        marker.center = center;
        marker.halfSizeX = halfSizeX;
        marker.halfSizeY = halfSizeY;
    }
}

void CMissionMarkerSync::InjectHooks()
{
    patch::ReplaceFunction(0x485E00, CTheScripts__HighlightImportantArea_Hook);
}

static bool IsSameMarker(const MissionMarkers::SMarker& a, const MissionMarkers::SMarker& b)
{
    return (a.center - b.center).Magnitude() < 0.5f && std::abs(a.halfSizeX - b.halfSizeX) < 0.1f &&
           std::abs(a.halfSizeY - b.halfSizeY) < 0.1f;
}

void CMissionMarkerSync::Process()
{
    unsigned int nTime = GetTickCount();

    // the network players draw the host's areas
    if (!CLocalPlayer::m_bIsHost && ms_nReceivedMarkers)
    {
        if (nTime > ms_nReceiveTime + 1500)
        {
            ms_nReceivedMarkers = 0;
        }

        for (int i = 0; i < ms_nReceivedMarkers; i++)
        {
            const auto& marker = ms_aReceivedMarkers[i];
            RenderArea(0xC00A0000 + i, marker.center, marker.halfSizeX, marker.halfSizeY);
        }
    }

    if (!CLocalPlayer::m_bIsHost)
        return;

    if (ms_bScriptHandlesCheckpoints && !CTheScripts::IsPlayerOnAMission())
    {
        ms_bScriptHandlesCheckpoints = false;
    }

    // the scripts are processed once per frame, so these are this frame's areas
    bool bChanged = ms_nFrameMarkers != ms_nSentMarkers;
    for (int i = 0; i < ms_nFrameMarkers && !bChanged; i++)
    {
        bChanged = !IsSameMarker(ms_aFrameMarkers[i], ms_aSentMarkers[i]);
    }

    // refreshed while they're shown, the network players hide them when they're not refreshed
    if (bChanged || (ms_nFrameMarkers && nTime > ms_nLastSendTime + 500))
    {
        MissionMarkers packet{};
        packet.count = static_cast<uint8_t>(ms_nFrameMarkers);
        for (int i = 0; i < ms_nFrameMarkers; i++)
        {
            packet.markers[i] = ms_aFrameMarkers[i];
            ms_aSentMarkers[i] = ms_aFrameMarkers[i];
        }
        ms_nSentMarkers = ms_nFrameMarkers;
        ms_nLastSendTime = nTime;

        GetPacketFactory().Send(packet);
    }

    ms_nFrameMarkers = 0;
}

void CMissionMarkerSync::HandlePacket(MissionMarkers* pMissionMarkers)
{
    if (CLocalPlayer::m_bIsHost)
        return;

    ms_nReceivedMarkers = std::min<int>(pMissionMarkers->count, MissionMarkers::MAX_MARKERS);
    for (int i = 0; i < ms_nReceivedMarkers; i++)
    {
        ms_aReceivedMarkers[i] = pMissionMarkers->markers[i];
    }
    ms_nReceiveTime = GetTickCount();
}
