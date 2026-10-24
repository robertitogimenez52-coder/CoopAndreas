#pragma once

// the fires put out with water (fire extinguisher, fire truck) are put out for everyone,
// the fires themselves are created by the synced explosions, projectiles and entities
class CFireSync
{
public:
    static void Process();
    static void HandlePacket(Packets::World::FireExtinguished* pFireExtinguished);
};
