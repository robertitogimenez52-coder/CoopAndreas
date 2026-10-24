#pragma once

// the network players see the areas highlighted by the host's mission scripts (the locate spheres),
// unless the script places the network players' checkpoints itself (Coop.UpdateCheckpointForNetworkPlayer)
class CMissionMarkerSync
{
public:
    static void InjectHooks();
    static void Process();
    static void HandlePacket(Packets::Blips::MissionMarkers* pMissionMarkers);

    // the script handles the network players' checkpoints, until the mission ends
    static inline bool ms_bScriptHandlesCheckpoints = false;
};
