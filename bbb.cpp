#include <stdint.h>
#include "pico/stdlib.h"
#include "bsp/board_api.h"
#include "Ntsc.h"
#include "Sound.h"
#include "ScanKeys.h"

static const uint8_t hit[] = {
    1,F4, 1,G4, 1,A4, 1,B4, 1,C5, 1,D5, 1,E5, 1,F5,
    0
};

static const uint8_t BGM_B[] = {
    N4,C4, N4,G4, N8,C4, N4,G4, N4,A4,
    N8,A4, N8,G4, N8,G4, N8,F4, N8,F4, N8,E4, N8,E4,
    N4,D4, N4,D4, N8,D4, N4,E4, N4P,D4,
    N2P,0,

    N4,C4, N4,G4, N8,C4, N4,G4, N4,A4,
    N8,A4, N8,G4, N8,G4, N8,F4, N8,F4, N8,E4, N8,E4,
    N4,F4, N4,F4, N8,F4, N4,A4, N4P,G4,
    N2P,0,

    N8,E4, N8,E4, N8,E4, N4,E4, N8,E4, N4,A4,
    N8,D4, N8,D4, N8,D4, N4,D4, N8,D4, N4,G4,
    N8,0, N8,A4, N8,0, N8,G4, N8,0, N8,F4, N8,0, N8,E4,
    N4,D4, N4,E4, N2,C4,

    0xff
};
static const uint8_t BGM_C[] = {
    N8,C3, N4,0, N4P,E3, N8,G3, N8,0, // 3
    N8,A2, N4,0, N4P,C3, N8,E3, N8,0, // 4
    N8,D3, N4,0, N4P,F3, N8,A3, N8,0, // 5
    N8,G2, N4,0, N4P,B2, N8,D3, N8,0, // 6

    N8,C3, N4,0, N4P,E3, N8,G3, N8,0, // 7
    N8,A2, N4,0, N4P,C3, N8,E3, N8,0, // 8
    N8,F3, N4,0, N8,F3, N8,G2, N4,0, N8,G2, // 9
    N8,C3, N4,0, N4P,E3, N8,G3, N8,0, // 10

    N8,C3, N4,0, N8,C3, N8,A2, N4,0, N8,A2, // 11
    N8,D3, N4,0, N8,D3, N8,G2, N4,0, N8,G2, // 12
    N8,0, N8,F2, N8,0, N8,F2, N8,0, N8,G2, N8,0, N8,G2,
    N8,C3, N4,0, N4P,E3, N8,G3, N8,0, // 14

    0xff
};

volatile uint8_t TimerCount;

void WaitTimer(uint8_t t)
{
    while (TimerCount < t) {
        tuh_task();
    }
    TimerCount = 0;
}

static int sx = 0, sy = 0;

int main()
{
    // stdio_init_all();
    InitKeys();
    InitSound();
    InitNtsc();

    for (auto i = 0; i < 1000; ++i) {
        Vram[i] = i % 0x40;
    }
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);

    StartMelody(BGM_B, BGM_C);

    auto y = 0;
    while (true) {
        tuh_task();
        if (ScanKeys() != 0) {
            --sx;
            ++Vram[0];
        }
        ShowSprite(1, 4, y, 1);
        ShowSprite(0, sx, sy, 0);
        // gpio_put(16, true);
        // sleep_ms(300);
        // gpio_put(16, false);
        // sleep_ms(300);
        ++y;
        // ++x;
        WaitTimer(60);
        StartMelody(hit);
    }
}
