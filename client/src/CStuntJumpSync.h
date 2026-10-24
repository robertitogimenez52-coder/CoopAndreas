#pragma once

// a unique stunt jump done by any player is done for everyone
class CStuntJumpSync
{
public:
    static void Process();
    static void HandlePacket(Packets::World::StuntJumpDone* pStuntJumpDone);
};
