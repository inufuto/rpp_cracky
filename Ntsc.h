#pragma once
#include "Config.h"
#include "Vram.h"

extern const uint8_t TilePattern[];
extern const uint8_t SpritePattern[];

static constexpr auto SpriteCount = 32;
static constexpr auto MaxHorizontalSpriteCount = 8;

struct SpriteAttribute {
    uint8_t y;
    uint8_t x;
    uint8_t pattern;
};
extern SpriteAttribute SpriteAttributes[];

extern void InitNtsc();
