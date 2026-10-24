#pragma once

class CWantedLevelSync
{
public:
    // sends the local wanted level whenever it changes
    static void Process();
    static void HandlePacket(Packets::Players::PlayerWantedLevel* pPlayerWantedLevel);

private:
    static inline int ms_nLastSentWantedLevel = 0;
    static inline unsigned int ms_nLastSentTime = 0;
};
