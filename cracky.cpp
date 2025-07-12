#include <stdint.h>
#include <pico/stdlib.h>
#include <bsp/board_api.h>
#include <tusb.h>
#include "Ntsc.h"
#include "Sound.h"
#include "Main.h"
#include "ScanKeys.h"
#include "Print.h"

volatile uint8_t TimerCount;

void WaitTimer(uint8_t t)
{
    while (TimerCount < t) {
        tuh_task();
    }
    TimerCount = 0;
}

int main()
{
    // stdio_init_all();
    InitKeys();
    InitSound();
    InitNtsc();
    Main();
}
