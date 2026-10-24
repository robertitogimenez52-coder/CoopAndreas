#pragma once

// shares the cheats that change the world (weather, time, traffic, peds, ...) and the toggled ones,
// the personal ones (weapons, health, money, vehicles, ...) stay local
class CCheatSync
{
public:
    static void InjectHooks();
    static void Process();
    static void HandlePacket(Packets::World::CheatSync* pCheatSync);
    static void OnCheatFunction(int cheatId);

private:
    static bool IsSyncedFunction(int cheatId);
    static bool IsSyncedToggle(int cheatId);
};
