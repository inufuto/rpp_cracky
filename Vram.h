#pragma once
#include <cstring>
#include "cate.h"

constexpr auto VramWidth = 40;
constexpr auto VramHeight = 25;
constexpr auto VramStep = 1;
constexpr auto VramRowSize = VramWidth;

extern uint8_t Vram[];

constexpr byte StatusHeight = 0;
constexpr byte StageTop = 0;

extern void ClearScreen();
inline ptr<byte> Put(ptr<byte> pVram, byte c) {
    *pVram++ = c;
    return pVram;
}
inline ptr<byte> VramPtr(byte x, byte y) {
    return Vram + y * VramWidth + x;
}
inline ptr<byte> PrintC(ptr<byte> pVram, byte c) {
    return Put(pVram, c - 0x20);
}
extern void Put2C(ptr<byte> pVram, byte c);
