#include <stdio.h>
#include "pico/stdlib.h"
#include "Video.h"

int main()
{
    Video::Initialize();


    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);
    while (true) {
        gpio_put(16, true);
        sleep_ms(500);
        gpio_put(16, false);
        sleep_ms(500);
    }
}
