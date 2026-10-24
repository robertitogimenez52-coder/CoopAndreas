#pragma once

#include <CRadar.h>

namespace Packets::Blips
{
class UpdateCheckpoint : public Packet
{
    DEFINE_PACKET_TYPE(UpdateCheckpoint, ePacketType::UPDATE_CHECKPOINT, ePacketChannel::EVENT);

public:
    playerid_t forWhoPlayerId{}; // TODO(v0.3.1-alpha): send C2S only
    WorldPositionCompressed position{};
    WorldPositionCompressed radius{};

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_playerid(stream, forWhoPlayerId);
        serialize_object(stream, position);
        serialize_object(stream, radius);
        return true;
    }
};

class RemoveCheckpoint : public Packet
{
    DEFINE_PACKET_TYPE(RemoveCheckpoint, ePacketType::REMOVE_CHECKPOINT, ePacketChannel::EVENT);

public:
    playerid_t forWhoPlayerId{}; // TODO(v0.3.1-alpha): send C2S only

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_playerid(stream, forWhoPlayerId);
        return true;
    }
};
// the areas the host's mission scripts highlight (CTheScripts::HighlightImportantArea), e.g. the locate spheres
class MissionMarkers : public Packet
{
    DEFINE_PACKET_TYPE(MissionMarkers, ePacketType::MISSION_MARKERS, ePacketChannel::EVENT);

public:
    static constexpr int MAX_MARKERS = 8;

    struct SMarker
    {
        WorldPositionCompressed center{};
        float halfSizeX = 0.0f;
        float halfSizeY = 0.0f;
    };

    uint8_t count = 0;
    SMarker markers[MAX_MARKERS]{};

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, count, 0, MAX_MARKERS);
        for (int i = 0; i < count; i++)
        {
            serialize_object(stream, markers[i].center);
            if (Stream::IsWriting)
            {
                markers[i].halfSizeX = std::clamp(markers[i].halfSizeX, 0.0f, 500.0f);
                markers[i].halfSizeY = std::clamp(markers[i].halfSizeY, 0.0f, 500.0f);
            }
            serialize_compressed_float(stream, markers[i].halfSizeX, 0.0f, 500.0f, 0.05f);
            serialize_compressed_float(stream, markers[i].halfSizeY, 0.0f, 500.0f, 0.05f);
        }
        return true;
    }
};

}  // namespace Packets::Blips
