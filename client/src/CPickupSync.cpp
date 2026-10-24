#include "stdafx.h"
#include "CPickupSync.h"
#include <CPickups.h>

static constexpr int NUM_PICKUPS = 620;  // CPickups::aPickUps

// the pickup was picked up by the local player if it was taken within this distance
static constexpr float LOCAL_PICK_UP_DISTANCE = 3.0f;
static constexpr float MATCH_DISTANCE = 1.5f;

static bool ms_abLastAvailable[NUM_PICKUPS] = {};
static bool ms_bInitialised = false;

static bool IsSyncedType(uint8_t type)
{
    switch (type)
    {
        case PICKUP_ON_STREET:
        case PICKUP_ON_STREET_SLOW:
        case PICKUP_ONCE:
        case PICKUP_ONCE_TIMEOUT:
        case PICKUP_ONCE_TIMEOUT_SLOW:
        case PICKUP_MONEY:
            return true;
        default:
            return false;
    }
}

static bool IsAvailable(CPickup& pickup)
{
    return IsSyncedType(pickup.m_nPickupType) && !pickup.m_nFlags.bDisabled;
}

static CVector GetPosition(CPickup& pickup)
{
    return CVector(pickup.GetXCoord(), pickup.GetYCoord(), pickup.GetZCoord());
}

void CPickupSync::Process()
{
    CPlayerPed* pLocalPlayer = FindPlayerPed(0);
    if (!pLocalPlayer)
        return;

    CVector vecPlayerPos = pLocalPlayer->GetPosition();

    for (int i = 0; i < NUM_PICKUPS; i++)
    {
        CPickup& pickup = CPickups::aPickUps[i];
        bool bAvailable = IsAvailable(pickup);

        // CPickup::Update removes (or disables until it regenerates) the pickup picked up by the player in focus
        if (ms_bInitialised && ms_abLastAvailable[i] && !bAvailable && pickup.m_nModelIndex > 0)
        {
            CVector vecPos = GetPosition(pickup);
            if ((vecPos - vecPlayerPos).Magnitude() < LOCAL_PICK_UP_DISTANCE)
            {
                Packets::World::PickupCollected packet{};
                packet.pos = vecPos;
                packet.modelId = pickup.m_nModelIndex;
                GetPacketFactory().Send(packet);
            }
        }

        ms_abLastAvailable[i] = bAvailable;
    }

    ms_bInitialised = true;
}

// does what CPickup::Update does when the pickup is picked up, without giving anything
void CPickupSync::HandlePacket(Packets::World::PickupCollected* pPickupCollected)
{
    CVector vecPos = pPickupCollected->pos;

    for (int i = 0; i < NUM_PICKUPS; i++)
    {
        CPickup& pickup = CPickups::aPickUps[i];

        if (!IsAvailable(pickup) || pickup.m_nModelIndex != pPickupCollected->modelId)
            continue;

        if ((GetPosition(pickup) - vecPos).Magnitude() > MATCH_DISTANCE)
            continue;

        switch (pickup.m_nPickupType)
        {
            case PICKUP_ON_STREET:
            case PICKUP_ON_STREET_SLOW:
                pickup.m_nRegenerationTime = CTimer::m_snTimeInMilliseconds +
                    (pickup.m_nPickupType == PICKUP_ON_STREET || pickup.m_nModelIndex == MODEL_BRIBE ? 30000 : 36000);
                pickup.GetRidOfObjects();
                pickup.m_nFlags.bDisabled = true;
                break;
            default:
                pickup.Remove();
                break;
        }

        // don't send it back
        ms_abLastAvailable[i] = false;
        break;
    }
}
