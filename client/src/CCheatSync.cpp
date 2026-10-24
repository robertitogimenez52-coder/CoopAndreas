#include "stdafx.h"
#include "CCheatSync.h"
#include <utility>

static constexpr int NUM_CHEATS = Packets::World::CheatSync::NUM_CHEATS;

// the original CCheat::m_aCheatFunctions, before the entries were replaced by `CheatFunction_Hook`
static void (*ms_apfnOriginal[NUM_CHEATS])() = {};

// the last known state of CCheat::m_aCheatsActive, to send the toggled cheats once
static bool ms_abLastActive[NUM_CHEATS] = {};

static bool ms_bApplyingNetworkCheat = false;

// the cheat functions that change the world for everyone
static const uintptr_t ms_aSyncedFunctions[] = {
    0x438F40,  // SunnyWeatherCheat
    0x438F50,  // ExtraSunnyWeatherCheat
    0x438F60,  // CloudyWeatherCheat
    0x438F70,  // RainyWeatherCheat
    0x438F80,  // FoggyWeatherCheat
    0x438F90,  // FastTimeCheat
    0x438FC0,  // SlowTimeCheat
    0x4390D0,  // PinkCarsCheat
    0x4390F0,  // BlackCarsCheat
    0x4391D0,  // ElvisLivesCheat
    0x439230,  // BeachPartyCheat
    0x439360,  // GangsCheat
    0x4393D0,  // GangLandCheat
    0x4393F0,  // LoveConquersAllCheat
    0x4394B0,  // AllCarsAreShitCheat
    0x4394E0,  // AllCarsAreGreatCheat
    0x439510,  // MidnightCheat
    0x439540,  // DuskCheat
    0x439570,  // StormCheat
    0x439590,  // SandstormCheat
    0x439710,  // RiotCheat
    0x439720,  // FunhouseCheat
    0x439B20,  // MayhemCheat
    0x439C70,  // EverybodyAttacksPlayerCheat
    0x439DD0,  // VillagePeopleCheat
    0x439E50,  // NinjaCheat
    0x439F60,  // CountrysideInvasionCheat
};

template <int I>
static void CheatFunction_Hook()
{
    CCheatSync::OnCheatFunction(I);
}

template <size_t... I>
static constexpr std::array<void (*)(), sizeof...(I)> MakeCheatFunctionHooks(std::index_sequence<I...>)
{
    return {&CheatFunction_Hook<I>...};
}

static constexpr auto ms_apfnHooks = MakeCheatFunctionHooks(std::make_index_sequence<NUM_CHEATS>{});

bool CCheatSync::IsSyncedFunction(int cheatId)
{
    uintptr_t pfn = reinterpret_cast<uintptr_t>(ms_apfnOriginal[cheatId]);
    if (pfn == 0)
        return false;

    for (uintptr_t synced : ms_aSyncedFunctions)
    {
        if (synced == pfn)
            return true;
    }

    return false;
}

// the cheats without a function are plain toggles (traffic, peds, physics, ...), the toggles of
// the synced functions (e.g. beach party) are kept in sync too
bool CCheatSync::IsSyncedToggle(int cheatId)
{
    return ms_apfnOriginal[cheatId] == nullptr || IsSyncedFunction(cheatId);
}

void CCheatSync::InjectHooks()
{
    for (int i = 0; i < NUM_CHEATS; i++)
    {
        ms_apfnOriginal[i] = CCheat::m_aCheatFunctions[i];

        if (IsSyncedFunction(i))
        {
            patch::SetPointer(reinterpret_cast<int>(&CCheat::m_aCheatFunctions[i]), ms_apfnHooks[i]);
        }
    }
}

void CCheatSync::OnCheatFunction(int cheatId)
{
    ms_apfnOriginal[cheatId]();

    if (CNetwork::m_bAuthenticated && !ms_bApplyingNetworkCheat)
    {
        Packets::World::CheatSync packet{};
        packet.cheatId = cheatId;
        packet.bFunction = true;
        GetPacketFactory().Send(packet);
    }

    // the function may toggle the cheat, it's sent with the function
    ms_abLastActive[cheatId] = CCheat::m_aCheatsActive[cheatId];
}

void CCheatSync::Process()
{
    for (int i = 0; i < NUM_CHEATS; i++)
    {
        if (CCheat::m_aCheatsActive[i] == ms_abLastActive[i])
            continue;

        ms_abLastActive[i] = CCheat::m_aCheatsActive[i];

        if (!IsSyncedToggle(i))
            continue;

        Packets::World::CheatSync packet{};
        packet.cheatId = i;
        packet.bFunction = false;
        packet.bActive = CCheat::m_aCheatsActive[i];
        GetPacketFactory().Send(packet);
    }
}

void CCheatSync::HandlePacket(Packets::World::CheatSync* pCheatSync)
{
    int cheatId = pCheatSync->cheatId;
    if (cheatId < 0 || cheatId >= NUM_CHEATS)
        return;

    if (pCheatSync->bFunction)
    {
        if (!IsSyncedFunction(cheatId))
            return;

        ms_bApplyingNetworkCheat = true;
        OnCheatFunction(cheatId);
        ms_bApplyingNetworkCheat = false;
    }
    else
    {
        if (!IsSyncedToggle(cheatId))
            return;

        CCheat::m_aCheatsActive[cheatId] = pCheatSync->bActive;
        ms_abLastActive[cheatId] = pCheatSync->bActive;
    }

    CCheat::m_bHasPlayerCheated = true;
}
