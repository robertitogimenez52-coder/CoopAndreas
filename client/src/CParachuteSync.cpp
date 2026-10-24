#include "stdafx.h"
#include "CParachuteSync.h"
#include <CObject.h>

using ParachuteAnim = Packets::Players::PlayerParachute;

struct SParachuteAnim
{
    const char* szName;
    float fBlend;
    bool bLoop;
};

// the same as the Object.PlayAnim calls of the PARACH script
static const SParachuteAnim ms_aAnims[ParachuteAnim::PARACHUTE_ANIM_COUNT] = {
    {nullptr, 0.0f, false},                  // PARACHUTE_REMOVED
    {"PARA_OPEN_O", 1000.0f, false},         // PARACHUTE_OPEN
    {"PARA_FLOAT_O", 2.0f, true},            // PARACHUTE_FLOAT
    {"PARA_DECEL_O", 1.0f, true},            // PARACHUTE_DECEL
    {"PARA_STEERL_O", 1.0f, true},           // PARACHUTE_STEER_LEFT
    {"PARA_STEERR_O", 1.0f, true},           // PARACHUTE_STEER_RIGHT
    {"PARA_LAND_O", 1000.0f, false},         // PARACHUTE_LAND
    {"PARA_LAND_WATER_O", 1.0f, false},      // PARACHUTE_LAND_WATER
};

void CParachuteSync::SendAnim(uint8_t anim)
{
    if (!CNetwork::m_bAuthenticated || anim >= ParachuteAnim::PARACHUTE_ANIM_COUNT)
        return;

    Packets::Players::PlayerParachute packet{};
    packet.anim = anim;
    GetPacketFactory().Send(packet);
}

void CParachuteSync::HandlePacket(Packets::Players::PlayerParachute* pPlayerParachute)
{
    CNetworkPlayer* pNetworkPlayer = CNetworkPlayerManager::GetPlayer(pPlayerParachute->senderPlayerid);
    if (!pNetworkPlayer)
        return;

    if (pPlayerParachute->anim == ParachuteAnim::PARACHUTE_REMOVED)
    {
        RemoveParachute(pNetworkPlayer);
        return;
    }

    // played in Process once the model and the anims are loaded
    pNetworkPlayer->m_nParachuteAnim = pPlayerParachute->anim;
    pNetworkPlayer->m_bParachuteAnimPending = true;
}

void CParachuteSync::RemoveParachute(CNetworkPlayer* pNetworkPlayer)
{
    pNetworkPlayer->m_bParachuteAnimPending = false;
    pNetworkPlayer->m_nParachuteAnim = ParachuteAnim::PARACHUTE_REMOVED;

    if (CObject* pObject = pNetworkPlayer->m_pParachute)
    {
        pObject->CleanUpOldReference((CEntity**)&pNetworkPlayer->m_pParachute);
        pNetworkPlayer->m_pParachute = nullptr;

        CWorld::Remove(pObject);
        delete pObject;
    }
}

static bool LoadParachute()
{
    static int nBlockId = CAnimManager::GetAnimationBlockIndex("parachute");

    bool bLoaded = true;

    if (CStreaming::ms_aInfoForModel[MODEL_PARACHUTE].m_nLoadState != LOADSTATE_LOADED)
    {
        CStreaming::RequestModel(MODEL_PARACHUTE, GAME_REQUIRED);
        bLoaded = false;
    }

    if (nBlockId >= 0 && !CAnimManager::ms_aAnimBlocks[nBlockId].bLoaded)
    {
        CStreaming::RequestModel(25575 + nBlockId, GAME_REQUIRED);  // IFPToModelId
        bLoaded = false;
    }

    return bLoaded && nBlockId >= 0;
}

void CParachuteSync::Process()
{
    for (auto pNetworkPlayer : CNetworkPlayerManager::m_pPlayers)
    {
        CPlayerPed* pPed = pNetworkPlayer->m_pPed;

        if (pNetworkPlayer->m_pParachute && (!pPed || pPed->m_nPedFlags.bInVehicle))
        {
            RemoveParachute(pNetworkPlayer);
            continue;
        }

        if (!pNetworkPlayer->m_bParachuteAnimPending || !pPed || !LoadParachute())
            continue;

        if (!pNetworkPlayer->m_pParachute)
        {
            // CObject::Create, the same way as CNetworkObject, so it doesn't become a network object
            CObject* pObject = plugin::CallAndReturn<CObject*, 0x5A1F60, int, bool>(MODEL_PARACHUTE, false);
            if (!pObject)
                continue;

            pObject->m_nObjectType = OBJECT_MISSION;
            pObject->SetPosn(pPed->GetPosition());
            CWorld::Add(pObject);

            pObject->AttachEntityToEntity(pPed, CVector(0.0f, 0.0f, 0.0f), CVector(0.0f, 0.0f, 0.0f));

            pNetworkPlayer->m_pParachute = pObject;
            pObject->RegisterReference((CEntity**)&pNetworkPlayer->m_pParachute);
        }

        const SParachuteAnim& anim = ms_aAnims[pNetworkPlayer->m_nParachuteAnim];
        Command<Commands::PLAY_OBJECT_ANIM>(CPools::GetObjectRef(pNetworkPlayer->m_pParachute), anim.szName, "PARACHUTE",
            anim.fBlend, anim.bLoop, true);

        pNetworkPlayer->m_bParachuteAnimPending = false;
    }
}
