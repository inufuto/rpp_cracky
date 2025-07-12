#include <stdint.h>
#include "Vram.h"
#include "Chars.h"

uint8_t Vram[VramWidth * VramHeight];

void ClearScreen() 
{
    for (auto pVram = Vram; pVram < Vram + VramWidth * VramHeight; ++pVram) {
        *pVram = Char_Space;
    }
}

void Put2C(ptr<byte> pVram, byte c)
{
    for (auto i = 0; i < 2; ++i) {
        for (auto j = 0; j < 2; ++j) {
            pVram = Put(pVram, c);
            ++c;
        }
        pVram += VramWidth - 2 * VramStep;
    }
}
