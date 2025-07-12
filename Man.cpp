#include "Man.h"
#include "ScanKeys.h"
#include "Chars.h"
#include "Sprite.h"
#include "Stage.h"
#include "Vram.h"
#include "Destruction.h"
#include "Main.h"
#include "Sound.h"

extern void _deb();

constexpr byte PatternMask = 0x0f;
constexpr byte Pattern_Left = (Pattern_Man_Left - Pattern_Man);
constexpr byte Pattern_Right = (Pattern_Man_Right - Pattern_Man);
constexpr byte Pattern_UpDown = (Pattern_Man_UpDown - Pattern_Man);

struct Direction {
    byte key;
    byte pattern;
    sbyte dx, dy;
};
static const Direction DirectionTable[] = {
    { Keys_Left, Pattern_Left, -1, 0 },
    { Keys_Right, Pattern_Right, 1, 0 },
    { Keys_Up, Pattern_UpDown, 0, -1 },
    { Keys_Down, Pattern_UpDown, 0, 1 },
};

Movable Man;
static constptr<Direction> pOldDirection;

static void Show()
{
    byte pattern;
    pattern = Man.status & PatternMask;
    if ((Man.status & Movable_Fall) == 0) {
        if (pattern == Pattern_UpDown) {
            byte seq;
            seq = (Man.y >> CoordShift) & 1;
            pattern += seq;
        }
        else if (Man.dx != 0) {
            byte seq;
            seq = (Man.x >> CoordShift) & 3;
            if (seq == 3) {
                seq = 1;
            }
            pattern += seq + 1;
        }
    }
    ShowSprite(&Man, pattern + Pattern_Man);
}


void InitMan()
{
    Man.sprite = Sprite_Man;
    Man.status = Movable_Live | Pattern_Man_Left;
    Man.dx = 0;
    Man.dy = 0;
    pOldDirection = DirectionTable;
    LocateMovable(&Man, pStage->start);
    Show();
}


void FallMan()
{
    if ((Man.status & Movable_Fall) != 0) {
        if (((Man.x | Man.y) & CoordMask) == 0) {
            if (FallMovable(&Man)) { //  && CanMove(&Man, Man.dx, 1)
                Man.dy = 1;
                Man.dx = 0;
            }
        }
        MoveMovable(&Man);
        Show();
    }
}


void MoveMan()
{
    if (((Man.x | Man.y) & CoordMask) == 0) {
        sbyte dx, dy;
        byte pattern;
        dx = 0; dy = 0;
        pattern = Man.status & PatternMask;
        if ((Man.status & Movable_Fall) == 0) {
            byte key;
            key = ScanKeys();
            auto pDirection = DirectionTable;
            for (auto i = 0; i < 4; ++i) {
                if ((pDirection->key & key) != 0) {
                    dx = pDirection->dx;
                    dy = pDirection->dy;
                    pattern = pDirection->pattern;
                    if (CanMove(&Man, dx, dy)) {
                        pOldDirection = pDirection;
                    }
                    else {
                        if (CanMove(&Man, pOldDirection->dx, pOldDirection->dy)) {
                            dx = pOldDirection->dx;
                            dy = pOldDirection->dy;
                            pattern = pOldDirection->pattern;
                        }
                        else {
                            dx = 0;
                            dy = 0;
                        }
                    }
                    break;
                }
                ++pDirection;
            }
        }
        Man.dx = dx;
        Man.dy = dy;
        Man.status = (Man.status & ~PatternMask) | pattern;
    }
    MoveMovable(&Man);
    Show();
    if (((Man.x | Man.y) & CoordMask) == 0) {
        byte x, y;
        x = Man.x >> CoordShift;
        y = Man.y >> CoordShift;
        if ((x & ColumnMask) == 0 && (y & FloorMask) == 1) {
            byte column, floor, cell;
            column = x >> ColumnShift;
            floor = y >> FloorShift;
            cell = GetCell(column, floor);
            if ((cell & Cell_LowerMask) == Cell_CrackedFloor) {
                StartDestruction(column, floor);
                AddScore(1);
            }
            else if ((cell & Cell_UpperMask) == Cell_Item) {
                ptr<byte> pVram;
                SetCell(column, floor, cell & Cell_LowerMask);
                pVram = VramPtr(x, y + StageTop);
                for (auto i = 0; i < 2; ++i) {
                    for (auto j = 0; j < 2; ++j) {
                        pVram = Put(pVram, Char_Space);
                    }
                    pVram += VramRowSize - 2 * VramStep;
                }
                --ItemCount;
                AddScore(5);
                Sound_Hit();
            }
        }
        FallMovable(&Man);
    }
}


void LooseMan()
{
    static const byte patterns[] = {
        Pattern_Man_Left,
        Pattern_Man_Loose0,
        Pattern_Man_Loose1,
        Pattern_Man_Loose2,
    };
    byte i;
    i = 0;
    do {
        ShowSprite(&Man, patterns[i & 3]);
        // UpdateSprites();
        Sound_Loose();
        ++i;
    } while (i < 8);
}
