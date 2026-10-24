#include "stdafx.h"
#include "CStuntJumpSync.h"
#include <CPool.h>
#include <CMessages.h>

// CStuntJump, see gta-reversed
struct CStuntJumpSA
{
    uint8_t m_aBoxesCameraReward[0x3C];  // CBoundingBox start, end, CVector camera
    int32_t m_nReward;
    bool m_bDone;
    bool m_bFound;
    uint8_t m_aPad[2];
};
VALIDATE_SIZE(CStuntJumpSA, 0x44);

static auto& mp_poolStuntJumps = *reinterpret_cast<CPool<CStuntJumpSA>**>(0xA9A888);
static auto& m_iNumJumps = *reinterpret_cast<int32_t*>(0xA9A89C);
static auto& m_iNumCompleted = *reinterpret_cast<uint32_t*>(0xA9A8A0);

static constexpr int MAX_STUNT_JUMPS = 256;
static bool ms_abLastDone[MAX_STUNT_JUMPS] = {};
static bool ms_bInitialised = false;

static CStuntJumpSA* GetJump(int index)
{
    auto pool = mp_poolStuntJumps;
    if (!pool || index < 0 || index >= pool->m_nSize || index >= MAX_STUNT_JUMPS || pool->m_byteMap[index].bEmpty)
        return nullptr;

    return &pool->m_pObjects[index];
}

void CStuntJumpSync::Process()
{
    auto pool = mp_poolStuntJumps;
    if (!pool)
        return;

    int nSize = std::min(pool->m_nSize, MAX_STUNT_JUMPS);

    // a game was loaded (or it's the first check), the jumps of the save are not sent
    int nChanged = 0;
    for (int i = 0; i < nSize; i++)
    {
        CStuntJumpSA* pJump = GetJump(i);
        if (pJump && pJump->m_bDone != ms_abLastDone[i])
            nChanged++;
    }

    bool bSend = ms_bInitialised && nChanged == 1;
    ms_bInitialised = true;

    if (nChanged == 0)
        return;

    for (int i = 0; i < nSize; i++)
    {
        CStuntJumpSA* pJump = GetJump(i);
        bool bDone = pJump && pJump->m_bDone;

        if (bDone == ms_abLastDone[i])
            continue;

        ms_abLastDone[i] = bDone;

        if (bSend && bDone)
        {
            Packets::World::StuntJumpDone packet{};
            packet.jumpIndex = static_cast<uint16_t>(i);
            GetPacketFactory().Send(packet);
        }
    }
}

// what CStuntJumpManager::Update does when the player lands the jump (0x49C4ED)
void CStuntJumpSync::HandlePacket(Packets::World::StuntJumpDone* pStuntJumpDone)
{
    int index = pStuntJumpDone->jumpIndex;
    CStuntJumpSA* pJump = GetJump(index);
    if (!pJump || pJump->m_bDone)
        return;

    pJump->m_bDone = true;
    pJump->m_bFound = true;
    ms_abLastDone[index] = true;

    ++m_iNumCompleted;
    CStats::IncrementStat(STAT_UNIQUE_JUMPS_DONE, 1.0f);

    if (!pStuntJumpDone->bReward)
        return;

    bool bAll = static_cast<int32_t>(m_iNumCompleted) == m_iNumJumps;
    int nReward = bAll ? 10000 : pJump->m_nReward;
    CWorld::Players[0].m_nMoney += nReward;

    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PART_MISSION_COMPLETE, 0.0f, 1.0f);

    if (bAll)
    {
        if (char* text = TheText.Get("USJ_ALL")) // ALL UNIQUE STUNTS COMPLETED!
            CHud::SetHelpMessage(text, false, false, false);
    }

    if (char* text = TheText.Get("USJ")) // UNIQUE STUNT BONUS!
        CMessages::AddBigMessageQ(text, 5000, 4);  // STYLE_MIDDLE_SMALLER_HIGHER

    if (char* text = TheText.Get("REWARD"))
        CMessages::AddBigMessageWithNumber(text, 6000, 5, nReward, -1, -1, -1, -1, -1);  // STYLE_WHITE_MIDDLE_SMALLER
}
