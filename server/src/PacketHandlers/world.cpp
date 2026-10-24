#include "network/packet_types.h"
#include "stdafx.h"
#include "network/packet_handler.h"
#include "network/packets/world.h"
#include "CCheatState.h"
#include "CStuntJumpState.h"
#include "CGangZoneState.h"

PACKET_HANDLER(
    ePacketType::GAME_WEATHER_TIME, Packets::World::GameWeatherTime* pGameWeatherTime, CNetworkPlayer* pNetworkPlayer)
{
	if (!pNetworkPlayer->m_bIsHost)
	{
		return;
	}

	GetPacketFactory().SendToAll(*pGameWeatherTime, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::ADD_EXPLOSION, Packets::World::AddExplosion* pAddExplosion, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pAddExplosion, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::TAG_UPDATE, Packets::World::TagUpdate* pTagUpdate, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pTagUpdate, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::UPDATE_ALL_TAGS, Packets::World::UpdateAllTags* pUpdateAllTags, CNetworkPlayer* pNetworkPlayer)
{
	if (pNetworkPlayer->m_bIsHost)
	{
		GetPacketFactory().SendToAll(*pUpdateAllTags, pNetworkPlayer);
	}
}

PACKET_HANDLER(ePacketType::UPDATE_MOON_SIZE, Packets::World::UpdateMoonSize* pUpdateMoonSize, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pUpdateMoonSize, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::CHEAT_SYNC, Packets::World::CheatSync* pCheatSync, CNetworkPlayer* pNetworkPlayer)
{
	if (!pCheatSync->bFunction)
	{
		CCheatState::ms_abActive[pCheatSync->cheatId] = pCheatSync->bActive;
	}

	GetPacketFactory().SendToAll(*pCheatSync, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::STUNT_JUMP_DONE, Packets::World::StuntJumpDone* pStuntJumpDone, CNetworkPlayer* pNetworkPlayer)
{
	if (!CStuntJumpState::ms_doneJumps.insert(pStuntJumpDone->jumpIndex).second)
	{
		return; // someone else has already done it
	}

	pStuntJumpDone->bReward = true;
	GetPacketFactory().SendToAll(*pStuntJumpDone, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::FIRE_EXTINGUISHED, Packets::World::FireExtinguished* pFireExtinguished, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pFireExtinguished, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::PICKUP_COLLECTED, Packets::World::PickupCollected* pPickupCollected, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pPickupCollected, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::GANG_ZONES_UPDATE, Packets::World::GangZonesUpdate* pGangZonesUpdate, CNetworkPlayer* pNetworkPlayer)
{
	if (!pNetworkPlayer->m_bIsHost)
	{
		return;
	}

	CGangZoneState::Store(*pGangZonesUpdate);
	GetPacketFactory().SendToAll(*pGangZonesUpdate, pNetworkPlayer);
}
