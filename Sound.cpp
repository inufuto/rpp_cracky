#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "Sound.h"

constexpr auto ChannelCount = 3;

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

// static uint pwmSlice;
static bool enabled;
Channel soundChannels[ChannelCount];
static volatile int time;

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

void StartMelody(const uint8_t *pMelody1, const uint8_t *pMelody2)
{
    enabled = false;
    soundChannels[1].pMelodyStart = soundChannels[1].pMelodyCurrent = pMelody1;
    soundChannels[1].noteLength = 1;
    soundChannels[2].pMelodyStart = soundChannels[2].pMelodyCurrent = pMelody2;
    soundChannels[2].noteLength = 1;
    enabled = true;
}

void StartMelody(const uint8_t *pMelody)
{
    enabled = false;
    soundChannels[0].pMelodyStart = soundChannels[0].pMelodyCurrent = pMelody;
    soundChannels[0].noteLength = 1;
    enabled = true;
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
        }
        time += 600 / 2;
    }
}
