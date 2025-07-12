#include <stdint.h>
#include <pico/stdlib.h>
#include <hardware/pwm.h>
#include <tusb.h>
#include "Sound.h"

constexpr auto ChannelCount = 3;
static constexpr auto StandardToneCycle = 24 * 2;
static constexpr auto PwmWrap = 0x100;
static constexpr auto PwmDivision = Config::SystemClock * 1000 / (440 * StandardToneCycle) / PwmWrap;
static constexpr auto ToneSampleCount = 32;

static const uint16_t cycles[] = {
    506,477,451,425,402,379,358,338,319,301,284,268,253,239,225,213,201,189,179,169,159,150,142,134,126,119,113,106,100,95,89,84,80,75,71,67,63,60,56,53
};
static const uint8_t wave1[] = {
    // SquareWave
	254,254,254,254,254,254,254,254,
	254,254,254,254,254,254,254,254,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};
static const uint8_t wave2[] = {
	// BassWave
	254,245,219,183,145,110,81,56,
	35,18,5,0,5,18,35,50,
	55,50,35,18,5,0,5,18,
	35,56,81,110,145,183,219,245,
};
static const uint8_t wave3[] = {
	// PianoWave
	254,238,204,172,150,128,104,84,
	74,65,50,36,32,33,25,9,
	0,9,25,33,32,36,50,65,
	74,84,104,128,150,172,204,238,
};
static uint8_t effectWave[] = {
	91,220,105,97,4,223,115,113,
	34,233,99,83,57,186,151,223,
	156,43,232,195,137,139,175,152,
	66,231,142,87,22,225,71,238,
	171,133,126,121,131,137,120,144,
	234,122,253,6,169,212,90,115,
	220,197,59,198,108,105,9,170,
	241,16,247,141,44,86,33,22,
	109,39,149,155,200,88,120,55,
	68,27,150,76,21,155,81,165,
	243,245,187,165,208,190,127,54,
	164,235,129,168,125,120,63,100,
	80,78,232,174,90,244,251,21,
	107,176,28,97,121,226,57,92,
	119,57,14,128,20,156,46,21,
	110,70,45,87,91,106,34,234,
};
static constexpr auto EffectSampleCount = sizeof effectWave;

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

static bool enabled;
static volatile Channel soundChannels[ChannelCount];
static volatile int time;
static volatile uint16_t effectOffset;
static volatile uint8_t effectVolume;

static void PwmHandler()
{
    auto pwmSlice = pwm_gpio_to_slice_num(Config::Gpio::Sound);
    pwm_clear_irq(pwmSlice);

    uint16_t sum = 0;
    for (auto& channel : soundChannels) {
        if (channel.volume != 0) {
            channel.denom -= StandardToneCycle;
            if (channel.denom < 0) {
                sum += static_cast<uint16_t>(channel.pWave[channel.offset]) * channel.volume;
                if (channel.offset++ >= ToneSampleCount) {
                    channel.offset = 0;
                }
                channel.denom += channel.cycle;
            }
        }
    }
    if (effectVolume != 0) {
        // gpio_put(16, true);
        auto offset = (effectOffset / 32) % EffectSampleCount;
        sum += effectWave[offset] * effectVolume;
        ++effectOffset;
    }
    auto level = sum / (ChannelCount / 2.5) / MaxVolume;
    if (level >= PwmWrap) {
        level = PwmWrap - 1;
    }
    pwm_set_gpio_level(Config::Gpio::Sound, level);
}

void InitSound()
{
    soundChannels[0].pWave = wave1;
    soundChannels[1].pWave = wave3;
    soundChannels[2].pWave = wave2;
    for (auto& channel : soundChannels) {
        channel.volume = 0;
        channel.denom = 0;
        channel.pMelodyCurrent = nullptr;
    }
    effectVolume = 0;
    time = 0;
    enabled = true;

    gpio_set_function(Config::Gpio::Sound, GPIO_FUNC_PWM);
    auto pwmSlice = pwm_gpio_to_slice_num(Config::Gpio::Sound);

    auto pwmConfig = pwm_get_default_config();
    pwm_init(pwmSlice, &pwmConfig, true);
    pwm_set_clkdiv(pwmSlice, PwmDivision);
    pwm_set_wrap(pwmSlice, PwmWrap - 1);

    pwm_set_gpio_level(Config::Gpio::Sound, 0);

    pwm_clear_irq(pwmSlice);
    pwm_set_irq_enabled(pwmSlice, true);
    irq_set_priority(PWM_IRQ_WRAP, 0x40);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, PwmHandler);
    irq_set_enabled(PWM_IRQ_WRAP, true);
}

void SoundHandler()
{
    if (!enabled) return;

    time -= Tempo;
    if (time < 0) {
        for (auto& channel : soundChannels) {
            if (channel.pMelodyCurrent != nullptr) {
                if (--channel.noteLength == 0) {
                    nextNote:
                    auto length = *channel.pMelodyCurrent++;
                    if (length == 0) {
                        //end
                        channel.pMelodyCurrent = nullptr;
                        channel.volume = 0;
                        continue;
                    }
                    if (length == 0xff) {
                        // repeat
                        channel.pMelodyCurrent = channel.pMelodyStart;
                        goto nextNote;
                    }
                    channel.noteLength = length;
                    auto tone = *channel.pMelodyCurrent++;
                    if (tone != 0) {
                        channel.cycle = cycles[tone - 1];
                        channel.volume = MaxVolume;
                    }
                    else {
                        channel.volume = 0;
                    }
                }
                else {
                    if (channel.volume != 0) {
                        --channel.volume;
                    }
                }
            }
            if (effectVolume != 0) {
                --effectVolume;
            }
        }
        time += 600 / 2;
    }
}


static void StartMelody(int index, const uint8_t *pMelody)
{
    soundChannels[index].pMelodyStart = soundChannels[index].pMelodyCurrent = pMelody;
    soundChannels[index].noteLength = 1;
}

static void WaitMelody(int index, const uint8_t *pMelody)
{
    enabled = false;
    StartMelody(index, pMelody);
    enabled = true;
    while (soundChannels[index].pMelodyCurrent != 0) {
        tuh_task();
    }
}

static void StartMelody(const uint8_t *pMelody)
{
    enabled = false;
    StartMelody(0, pMelody);
    enabled = true;
}

void Sound_Loose()
{
    static const uint8_t notes[] = {
        1,A3, 0
    };
    WaitMelody(0, notes);
}

void Sound_Hit()
{
    static const uint8_t notes[] = {
        1,F4, 1,G4, 1,A4, 1,B4, 1,C5, 1,D5, 1,E5, 1,F5,
        0
    };
    StartMelody(notes);
}

void Sound_Beep()
{
    static const uint8_t notes[] = {
        1,A4, 0
    };
    WaitMelody(0, notes);
}


void Sound_Start()
{
    static const uint8_t notes[] = {
        N8,0, N8,C5, N8,C5, N8,C5, N8,C5, N4,G4, N4,C5,
        N8,C5, N8,D5, N8,C5, N4,D5, N4,E5,
        N1,C5,
        0
    };
    WaitMelody(1, notes);
}

void Sound_Clear() 
{
    static const uint8_t notes[] = {
        N8,C4, N8,E4, N8,G4, N8,D4, N8,F4, N8,A4, N8,E4, N8,G4, N8,B4, N4P,C5,0,
        0
    };
    WaitMelody(1, notes);
}

void Sound_GameOver()
{
    static const uint8_t notes[] = {
        N8,C5, N8,C5, N8,G4, N8,G4, N8,A4, N8,A4, N8,B4, N8,B4,
        N2P,C5, N4,0,
        0
    };
    WaitMelody(1, notes);
}


void StartBGM()
{
    static const uint8_t notes1[] = {
        N8,0, N8,C5, N8,C5, N8,C5, N8,C5, N4,G4, N4,C5,
        N8,C5, N8,D5, N8,C5, N4,D5, N4,E5,
        N8,0, N8,C5, N8,C5, N8,C5, N8,C5, N4,D5, N4,F5,
        N8,F5, N8,E5, N8,C5, N4,C5, N4,D5,
        N8,0, N8,C5, N8,C5, N8,C5, N8,C5, N4,G4, N4,C5,
        N8,C5, N8,D5, N8,C5, N4,D5, N4,E5,
        N4,F5, N4,F5, N4,E5, N4,E5,
        N4,D5, N8,D5, N4,E5, N8,E5, N8,D5, N8,C5,
        N4,C5, N4,C5, N8,D5, N4,D5, N4P,C5, N2P,0,
        0xff
    };
    static const uint8_t notes2[] = {
        N8,C3, N4,0, N4P,E3, N8,G3, N8,0, // 3
        N8,A2, N4,0, N4P,C3, N8,E3, N8,0, // 4
        N8,D3, N4,0, N4P,F3, N8,A3, N8,0, // 5
        N8,G2, N4,0, N4P,B2, N8,D3, N8,0, // 6
        N8,C3, N4,0, N4P,E3, N8,G3, N8,0, // 7
        N8,A2, N4,0, N4P,C3, N8,E3, N8,0, // 8
        N8,F2, N4,0, N4P,A2, N8,C3, N8,0, // 9
        N8,D3, N4,0, N4P,E3, N8,A3, N8,0, // 10
        N8,F2, N4,0, N8,F2, N8,G2, N8,0, N8,B2, N8,D3, // 11
        N8,C3, N4,0, N4P,E3, N8,G3, N8,0, // 12
        0xff
    };
    enabled = false;
    StartMelody(1, notes1);
    StartMelody(2, notes2);
    enabled = true;
}

static void Stop(int index)
{
    soundChannels[index].pMelodyCurrent = nullptr;
    soundChannels[index].volume = 0;
}

void StopBGM()
{
    Stop(1);
    Stop(2);
}

// void StartEffect()
// {
//     enabled = false;
//     effectVolume = MaxVolume * 3 / 2;
//     effectOffset = 0;
//     enabled = true;
// }