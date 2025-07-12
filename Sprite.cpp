#include "Sprite.h"
#include "Ntsc.h"

constexpr auto InvalidY = 200;

void HideAllSprites()
{
    for (auto pSprite = SpriteAttributes; pSprite < SpriteAttributes + SpriteCount; ++pSprite) {
        pSprite->y = InvalidY;
    }
}


void ShowSprite(ptr<Movable> pMovable, byte pattern)
{
    auto pSprite = SpriteAttributes + pMovable->sprite;
    pSprite->x = pMovable->x >> 1;
    pSprite->y = pMovable->y;
    pSprite->pattern = pattern;
}


void HideSprite(byte index) 
{
    auto pSprite = SpriteAttributes + index;
    pSprite->y = InvalidY;
}
