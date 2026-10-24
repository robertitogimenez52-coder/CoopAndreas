#pragma once
#include "CWeather.h"
#include <CExplosion.h>

namespace Packets::World
{
// TODO: proper weather sync
class GameWeatherTime : public Packet
{
    DEFINE_PACKET_TYPE(GameWeatherTime, ePacketType::GAME_WEATHER_TIME, ePacketChannel::EVENT);

public:
    eWeatherType newWeather;
    eWeatherType oldWeather;
    // eWeatherType forcedWeather;
    uint8_t currentMonth;
    uint8_t currentDay;
    uint8_t currentHour;
    uint8_t currentMinute;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        if (Stream::IsWriting)
        {
            newWeather = VCLAMP(newWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
            oldWeather = VCLAMP(oldWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
            // forcedWeather = VCLAMP(forcedWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
            currentMonth = VCLAMP(currentMonth, 1, 12);
            currentDay = VCLAMP(currentDay, 1, 31);
            currentHour = VCLAMP(currentHour, 0, 23);
            currentMinute = VCLAMP(currentMinute, 0, 59);
        }
        serialize_int(stream, (int&)newWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
        serialize_int(stream, (int&)oldWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);
        // serialize_int(stream, (int&)forcedWeather, WEATHER_EXTRASUNNY_LA, WEATHER_EXTRACOLOURS_2);

        serialize_int(stream, currentMonth, 1, 12);
        serialize_int(stream, currentDay, 1, 31);
        serialize_int(stream, currentHour, 0, 23);
        serialize_int(stream, currentMinute, 0, 59);

        return true;
    }
};

// unused for now
class AddExplosion : public Packet
{
    DEFINE_PACKET_TYPE(AddExplosion, ePacketType::ADD_EXPLOSION, ePacketChannel::EVENT);

public:
    eExplosionType type;
    WorldPositionCompressed pos;
    int time;
    bool usesSound;
    float cameraShake;
    bool isVisible;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, (int&)type, EXPLOSION_GRENADE, EXPLOSION_RC_VEHICLE);
        serialize_object(stream, pos);
        serialize_int(stream, time, 0, 100);
        serialize_bool(stream, usesSound);
        serialize_compressed_float(stream, cameraShake, -1.0f, 1.0f, 0.01f);
        serialize_bool(stream, isVisible);

        return true;
    }
};

class TagUpdate : public Packet
{
    DEFINE_PACKET_TYPE(TagUpdate, ePacketType::TAG_UPDATE, ePacketChannel::EVENT);

public:
    struct Payload
    {
        int16_t pos_x = 0;
        int16_t pos_y = 0;
        int16_t pos_z = 0;
        bool bFullySprayed = false;
        uint8_t alpha = 0;
    } payload;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, payload.pos_x, -3000, 3000);
        serialize_int(stream, payload.pos_y, -3000, 3000);
        serialize_int(stream, payload.pos_z, -3000, 3000);

        serialize_bool(stream, payload.bFullySprayed);
        if (payload.bFullySprayed)
        {
            payload.alpha = 255;
        }
        else
        {
            if (Stream::IsWriting)
            {
                uint8_t temp = payload.alpha / 8;
                serialize_int(stream, temp, 0, 31);
            }
            else if (Stream::IsReading)
            {
                uint8_t temp;
                serialize_int(stream, temp, 0, 31);
                payload.alpha = temp * 8;
            }
        }

        return true;
    }
};

class UpdateAllTags : public Packet
{
    DEFINE_PACKET_TYPE(UpdateAllTags, ePacketType::UPDATE_ALL_TAGS, ePacketChannel::EVENT);

public:
    TagUpdate::Payload tags[100];

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        for (size_t i = 0; i < ARRAY_SIZE(tags); i++)
        {
            serialize_int(stream, tags[i].pos_x, -3000, 3000);
            serialize_int(stream, tags[i].pos_y, -3000, 3000);
            serialize_int(stream, tags[i].pos_z, -3000, 3000);
            bool bIsNull = false;
            bool bFullySprayed = false;
            if (Stream::IsWriting)
            {
                if (tags[i].alpha == 0)
                {
                    bIsNull = true;
                }
                serialize_bool(stream, bIsNull);
                if (bIsNull)
                {
                    continue;
                }

                if (tags[i].alpha == 255)
                {
                    bFullySprayed = true;
                }
                serialize_bool(stream, bFullySprayed);
                if (bFullySprayed)
                {
                    continue;
                }
                uint8_t temp = tags[i].alpha / 8;
                serialize_int(stream, temp, 0, 31);
            }
            else if (Stream::IsReading)
            {
                serialize_bool(stream, bIsNull);
                if (bIsNull)
                {
                    tags[i].alpha = 0;
                    continue;
                }

                serialize_bool(stream, bFullySprayed);
                if (bFullySprayed)
                {
                    tags[i].alpha = 255;
                    continue;
                }

                uint8_t temp;
                serialize_int(stream, temp, 0, 31);
                tags[i].alpha = temp * 8;
            }
        }
        return true;
    }
};

class UpdateMoonSize : public Packet
{
    DEFINE_PACKET_TYPE(UpdateMoonSize, ePacketType::UPDATE_MOON_SIZE, ePacketChannel::EVENT);

public:
    uint8_t moonSize;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, moonSize, 0, 7);
        return true;
    }
};

class CheatSync : public Packet
{
    DEFINE_PACKET_TYPE(CheatSync, ePacketType::CHEAT_SYNC, ePacketChannel::EVENT);

public:
    static constexpr uint8_t NUM_CHEATS = 92;

    uint8_t cheatId = 0;

    // true - the cheat function was called (weather, time, peds, traffic, ...), false - the cheat was toggled
    bool bFunction = false;
    bool bActive = false;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_int(stream, cheatId, 0, NUM_CHEATS - 1);
        serialize_bool(stream, bFunction);
        serialize_bool(stream, bActive);
        return true;
    }
};

class StuntJumpDone : public Packet
{
    DEFINE_PACKET_TYPE(StuntJumpDone, ePacketType::STUNT_JUMP_DONE, ePacketChannel::EVENT);

public:
    // the index in CStuntJumpManager::mp_poolStuntJumps, the same for everyone as they're loaded from the data files
    uint16_t jumpIndex = 0;

    // false when it's sent to a player who joined later
    bool bReward = true;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_uint16(stream, jumpIndex);
        serialize_bool(stream, bReward);
        return true;
    }
};

class FireExtinguished : public Packet
{
    DEFINE_PACKET_TYPE(FireExtinguished, ePacketType::FIRE_EXTINGUISHED, ePacketChannel::EVENT);

public:
    WorldPositionCompressed pos{};

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_object(stream, pos);
        return true;
    }
};

class PickupCollected : public Packet
{
    DEFINE_PACKET_TYPE(PickupCollected, ePacketType::PICKUP_COLLECTED, ePacketChannel::EVENT);

public:
    // the pickups are matched by the position and model, their indices are different for everyone
    WorldPositionCompressed pos{};
    int16_t modelId = 0;

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_object(stream, pos);
        serialize_int(stream, modelId, 0, 20000);
        return true;
    }
};

// the gang territories (CZoneInfo::m_nGangDensity), sent by the host
class GangZonesUpdate : public Packet
{
    DEFINE_PACKET_TYPE(GangZonesUpdate, ePacketType::GANG_ZONES_UPDATE, ePacketChannel::EVENT);

public:
    static constexpr int MAX_ZONES = 64;      // per packet
    static constexpr int NUM_ZONE_INFOS = 380;  // CTheZones::ZoneInfoArray
    static constexpr int NUM_GANGS = 10;

    struct SZone
    {
        uint16_t index = 0;
        uint8_t gangDensity[NUM_GANGS]{};
    };

    bool bGangWarsActive = false;
    uint8_t count = 0;
    SZone zones[MAX_ZONES]{};

private:
    template <typename Stream>
    bool Serialize(Stream& stream)
    {
        serialize_bool(stream, bGangWarsActive);
        serialize_int(stream, count, 0, MAX_ZONES);
        for (int i = 0; i < count; i++)
        {
            serialize_int(stream, zones[i].index, 0, NUM_ZONE_INFOS - 1);
            for (int j = 0; j < NUM_GANGS; j++)
            {
                serialize_uint8(stream, zones[i].gangDensity[j]);
            }
        }
        return true;
    }
};

}  // namespace Packets::World
