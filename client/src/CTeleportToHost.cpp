#include "stdafx.h"
#include "CTeleportToHost.h"

static constexpr unsigned int TELEPORT_COOLDOWN = 5000;

void CTeleportToHost::Teleport()
{
    if (CLocalPlayer::m_bIsHost)
    {
        CChat::AddMessage("{d6dce8}[CoopAndreas] You're the host, the others can use /tp to come to you.");
        return;
    }

    if (!CTheScripts::IsPlayerOnAMission())
    {
        CChat::AddMessage("{d6dce8}[CoopAndreas] /tp only works during a mission.");
        return;
    }

    unsigned int nTime = GetTickCount();
    if (ms_nLastTeleportTime && nTime < ms_nLastTeleportTime + TELEPORT_COOLDOWN)
    {
        CChat::AddMessage("{d6dce8}[CoopAndreas] Wait a few seconds before using /tp again.");
        return;
    }

    CNetworkPlayer* pHost = nullptr;
    for (auto pNetworkPlayer : CNetworkPlayerManager::m_pPlayers)
    {
        if (pNetworkPlayer->m_bIsHost)
        {
            pHost = pNetworkPlayer;
            break;
        }
    }

    CPlayerPed* pLocalPlayer = FindPlayerPed(0);
    if (!pHost || !pHost->m_pPed || !pLocalPlayer || pLocalPlayer->m_fHealth <= 0.0f)
    {
        CChat::AddMessage("{d6dce8}[CoopAndreas] The host can't be found.");
        return;
    }

    ms_nLastTeleportTime = nTime;

    CPlayerPed* pHostPed = pHost->m_pPed;
    int nLocalRef = CPools::GetPedRef(pLocalPlayer);

    if (pLocalPlayer->m_nPedFlags.bInVehicle && pLocalPlayer->m_pVehicle)
    {
        CVector pos = pLocalPlayer->GetPosition();
        Command<Commands::WARP_CHAR_FROM_CAR_TO_COORD>(nLocalRef, pos.x, pos.y, pos.z);
    }

    // the host's interior
    int nArea = pHostPed->m_nAreaCode;
    if (CGame::currArea != nArea)
    {
        Command<Commands::SET_AREA_VISIBLE>(nArea);
    }
    Command<Commands::SET_CHAR_AREA_VISIBLE>(nLocalRef, nArea);

    // a free seat in the host's vehicle
    CVehicle* pVehicle = pHostPed->m_nPedFlags.bInVehicle ? pHostPed->m_pVehicle : nullptr;
    if (pVehicle && CNetworkVehicleManager::GetVehicle(pVehicle))
    {
        for (int i = 0; i < pVehicle->m_nMaxPassengers; i++)
        {
            if (pVehicle->m_apPassengers[i] == nullptr)
            {
                Command<Commands::WARP_CHAR_INTO_CAR_AS_PASSENGER>(nLocalRef, CPools::GetVehicleRef(pVehicle), i);
                return;
            }
        }
    }

    // behind the host, or beside the host's vehicle
    CVector pos = pHostPed->GetPosition();
    float fHeading = pVehicle ? pVehicle->GetHeading() : pHostPed->m_fCurrentRotation;
    float fDistance = pVehicle ? 3.5f : 1.5f;
    pos.x += sinf(fHeading) * fDistance;
    pos.y -= cosf(fHeading) * fDistance;

    bool bFound = false;
    float fGroundZ = CWorld::FindGroundZFor3DCoord(pos.x, pos.y, pos.z + 2.0f, &bFound, nullptr);
    if (bFound)
    {
        pos.z = fGroundZ + 1.0f;
    }

    Command<Commands::LOAD_SCENE>(pos.x, pos.y, pos.z);
    pLocalPlayer->Teleport(pos, false);
    pLocalPlayer->m_fCurrentRotation = pLocalPlayer->m_fAimingRotation = fHeading;
}
