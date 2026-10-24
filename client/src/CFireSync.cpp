#include "stdafx.h"
#include "CFireSync.h"
#include <CFireManager.h>

// the fires that were being put out with water on the last check
static bool ms_abWasBeingExtinguished[MAX_NUM_FIRES] = {};
static CVector ms_avecLastPosition[MAX_NUM_FIRES] = {};

void CFireSync::Process()
{
    for (int i = 0; i < MAX_NUM_FIRES; i++)
    {
        CFire& fire = gFireManager.m_aFires[i];

        // CFire::Extinguish clears both flags, so it went out while the water was hitting it
        if (!fire.m_nFlags.bActive && ms_abWasBeingExtinguished[i])
        {
            Packets::World::FireExtinguished packet{};
            packet.pos = ms_avecLastPosition[i];
            GetPacketFactory().Send(packet);
        }

        ms_abWasBeingExtinguished[i] = fire.m_nFlags.bActive && fire.m_nFlags.bBeingExtinguished;
        ms_avecLastPosition[i] = fire.m_vecPosition;
    }
}

void CFireSync::HandlePacket(Packets::World::FireExtinguished* pFireExtinguished)
{
    CVector pos = pFireExtinguished->pos;
    gFireManager.ExtinguishPoint(pos, 2.0f);

    // don't send it back
    for (int i = 0; i < MAX_NUM_FIRES; i++)
    {
        if (!gFireManager.m_aFires[i].m_nFlags.bActive)
            ms_abWasBeingExtinguished[i] = false;
    }
}
