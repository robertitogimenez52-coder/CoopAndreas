#pragma once

// the "/tp" chat command, a network player left behind on a mission goes to the host
class CTeleportToHost
{
public:
    static void Teleport();

private:
    static inline unsigned int ms_nLastTeleportTime = 0;
};
