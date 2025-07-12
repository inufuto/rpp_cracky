#include "Destruction.h"
#include "Stage.h"
#include "Vram.h"
#include "Chars.h"

extern void _deb();

constexpr byte MaxCount = 3;
constexpr byte MaxTime = 3;

struct Destruction {
    byte column, floor;
    byte time;
};
Destruction Destructions[MaxCount];
static byte clock;

void InitDestructions()
{
    for (auto pDestruction = Destructions; pDestruction < Destructions + MaxCount; ++pDestruction) {
        pDestruction->time = 0;
    }
    clock = 0;
}

void StartDestruction(byte column, byte floor)
{
    for (auto pDestruction = Destructions; pDestruction < Destructions + MaxCount; ++pDestruction) {
        if (pDestruction->time == 0) {
            pDestruction->column = column;
            pDestruction->floor = floor;
            pDestruction->time = MaxTime;
            SetCell(column, floor, Cell_BrokenFloor | Cell_HardFloor);
            return;
        }
    }
}


void UpdateDestructions()
{
    ++clock;
    if ((clock & CoordMask) != 0) return;
    for (auto pDestruction = Destructions; pDestruction < Destructions + MaxCount; ++pDestruction) {
        if (pDestruction->time != 0) {
            byte time;
            byte x, y;
            time = pDestruction->time - 1;
            pDestruction->time = time;
            x = pDestruction->column << 1;
            y = ((pDestruction->floor << 2) + (FloorHeight - 1));
            {
                ptr<byte> pVram;
                byte c;
                pVram = VramPtr(x, y + StageTop);
                c = Char_CrackedFloor + ((MaxTime - time) << 1);
                for (auto i = 0; i < 2; ++i) {
                    pVram = Put(pVram, c);
                    ++c;
                }
            }
            if (time == 0) {
                SetCell(pDestruction->column, pDestruction->floor, Cell_Space);
            }
        }
    }
}