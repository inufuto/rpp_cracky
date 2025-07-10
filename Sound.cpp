#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "Sound.h"

uint Sound::pwmSlice;
bool Sound::enabled;
Sound::Channel Sound::channels[ChannelCount];
int Sound::time;
const uint16_t Sound::cycles[] = {
    506,477,451,425,402,379,358,338,319,301,284,268,253,239,225,213,201,189,179,169,159,150,142,134,126,119,113,106,100,95,89,84,80,75,71,67,63,60,56,53
};

void Sound::Initialize()
{
    for (auto& channel : channels) {
        channel.volume = 0;
        channel.denom = 0;
        channel.pMelodyCurrent = nullptr;
    }
    time = 0;
    enabled = true;

    gpio_set_function(Config::Gpio::Sound, GPIO_FUNC_PWM);
    pwmSlice = pwm_gpio_to_slice_num(Config::Gpio::Sound);

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

void Sound::SetWave(int channelIndex, const uint8_t *pWave)
{
    channels[channelIndex].pWave = pWave;
}

void Sound::StartMelody(const uint8_t *pMelody1, const uint8_t *pMelody2)
{
    enabled = false;
    channels[1].pMelodyStart = channels[1].pMelodyCurrent = pMelody1;
    channels[1].noteLength = 1;
    channels[2].pMelodyStart = channels[2].pMelodyCurrent = pMelody2;
    channels[2].noteLength = 1;
    enabled = true;
}

void Sound::PwmHandler()
{
    pwm_clear_irq(pwmSlice);

    uint16_t sum = 0;
    for (auto& channel : channels) {
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
    auto level = sum / (ChannelCount / 2) / MaxVolume;
    gpio_put(16, level >= PwmWrap);
    if (level >= PwmWrap) {
        level = PwmWrap - 1;
    }
    pwm_set_gpio_level(Config::Gpio::Sound, level);
}

void Sound::MelodyHandler()
{
    if (!enabled) return;

    time -= Tempo;
    if (time < 0) {
        for (auto& channel : channels) {
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
