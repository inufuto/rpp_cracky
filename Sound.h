#pragma once
#include "Config.h"

class Sound {
private:
    struct Channel {
        const uint8_t* pWave;
        uint8_t offset;
        uint8_t volume;
        int16_t denom;
        int16_t cycle;
        const uint8_t* pMelodyStart;
        const uint8_t* pMelodyCurrent;
        uint8_t noteLength;
    };
private:
    static constexpr auto Tempo = 160;
    static constexpr auto ChannelCount = 3;

    static constexpr auto StandardToneCycle = 24 * 2;
    static constexpr auto PwmWrap = 0x100;
    static constexpr auto PwmDivision = Config::SystemClock * 1000 / (440 * StandardToneCycle) / PwmWrap;

    static constexpr auto ToneSampleCount = 32;
    static constexpr auto MaxVolume = 15;
    static constexpr auto VolumeRate = 2;
private:
    static uint pwmSlice;
    static uint pwmChannel;
    static bool enabled;
    static Channel channels[ChannelCount];
    static int time;
    static const uint16_t cycles[];
public:
    static void Initialize();
    static void SetWave(int channelIndex, const uint8_t* pWave);
    static void MelodyHandler();
    static void StartMelody(const uint8_t* pMelody1, const uint8_t* pMelody2);
private:
    static void PwmHandler();
};

constexpr auto N8 = 6;
constexpr auto N8L = 8;
constexpr auto N8R = 4;
constexpr auto N8P = N8 * 3 / 2;
constexpr auto N4 = N8 * 2;
constexpr auto N4P = N4 * 3 / 2;
constexpr auto N2 = N4 * 2;
constexpr auto N2P = N2 * 3 / 2;
constexpr auto N1 = N2 * 2;

constexpr auto E2 = 1;
constexpr auto F2 = 2;
constexpr auto F2S = 3;
constexpr auto G2 = 4;
constexpr auto G2S = 5;
constexpr auto A2 = 6;
constexpr auto A2S = 7;
constexpr auto B2 = 8;
constexpr auto C3 = 9;
constexpr auto C3S = 10;
constexpr auto D3 = 11;
constexpr auto D3S = 12;
constexpr auto E3 = 13;
constexpr auto F3 = 14;
constexpr auto F3S = 15;
constexpr auto G3 = 16;
constexpr auto G3S = 17;
constexpr auto A3 = 18;
constexpr auto A3S = 19;
constexpr auto B3 = 20;
constexpr auto C4 = 21;
constexpr auto C4S = 22;
constexpr auto D4 = 23;
constexpr auto D4S = 24;
constexpr auto E4 = 25;
constexpr auto F4 = 26;
constexpr auto F4S = 27;
constexpr auto G4 = 28;
constexpr auto G4S = 29;
constexpr auto A4 = 30;
constexpr auto A4S = 31;
constexpr auto B4 = 32;
constexpr auto C5 = 33;
constexpr auto C5S = 34;
constexpr auto D5 = 35;
constexpr auto D5S = 36;
constexpr auto E5 = 37;
constexpr auto F5 = 38;
constexpr auto F5S = 39;
constexpr auto G5 = 40;
