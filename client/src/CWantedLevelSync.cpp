#include "stdafx.h"
#include "CWantedLevelSync.h"
#include <CWanted.h>

void CWantedLevelSync::Process()
{
    CWanted* pWanted = FindPlayerWanted(0);
    if (!pWanted)
        return;

    int nWantedLevel = std::clamp((int)pWanted->m_nWantedLevel, 0, 6);
    unsigned int nTime = GetTickCount();

    // also resent from time to time, e.g. the server forgets it after a reconnect
    if (nWantedLevel == ms_nLastSentWantedLevel && (nWantedLevel == 0 || nTime < ms_nLastSentTime + 5000))
        return;

    ms_nLastSentWantedLevel = nWantedLevel;
    ms_nLastSentTime = nTime;

    Packets::Players::PlayerWantedLevel packet{};
    packet.wantedLevel = nWantedLevel;
    GetPacketFactory().Send(packet);
}

void CWantedLevelSync::HandlePacket(Packets::Players::PlayerWantedLevel* pPlayerWantedLevel)
{
    CNetworkPlayer* pNetworkPlayer = CNetworkPlayerManager::GetPlayer(pPlayerWantedLevel->senderPlayerid);
    if (!pNetworkPlayer)
        return;

    pNetworkPlayer->m_nWantedLevel = pPlayerWantedLevel->wantedLevel;

    // the network player's own CWanted, so the scripts can check it with the internal player id,
    // the cops are only generated for the local player's one
    int playerNum = pNetworkPlayer->GetInternalId();
    if (playerNum > 0)
    {
        if (CWanted* pWanted = CWorld::Players[playerNum].m_PlayerData.m_pWanted)
        {
            pWanted->m_nWantedLevel = pPlayerWantedLevel->wantedLevel;
        }
    }
}
