#pragma once

// a pickup in the world (weapons, health, armour, bribes, money) picked up by a player is gone for everyone,
// the collectables, properties and the pickups created by the scripts are left alone
class CPickupSync
{
public:
    static void Process();
    static void HandlePacket(Packets::World::PickupCollected* pPickupCollected);
};
