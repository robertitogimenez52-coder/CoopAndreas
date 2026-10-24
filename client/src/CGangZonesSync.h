#pragma once

// the host owns the gang territories and the gang wars, the others get the territories from it
class CGangZonesSync
{
public:
    static void InjectHooks();
    static void Process();
    static void HandlePacket(Packets::World::GangZonesUpdate* pGangZonesUpdate);
};
