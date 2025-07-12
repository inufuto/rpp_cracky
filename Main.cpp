#include <tusb.h>
#include "Main.h"
#include "ScanKeys.h"
#include "Vram.h"
#include "Sound.h"
#include "Sprite.h"
#include "Status.h"
#include "Stage.h"
#include "Man.h"
#include "Destruction.h"
#include "Monster.h"

constexpr byte MaxTimeDenom = 50 / (8 / CoordRate);
constexpr byte BonusRate = 8;

word Score;
word HiScore;
byte RemainCount;
byte CurrentStage;
byte StageTime;
byte ItemCount;
static byte Clock;
static sbyte monsterNum;

extern void _deb();
void Main()
{
    byte key, timeDenom;

    HiScore = 0;
    Score = 0;
    CurrentStage = 0;
    RemainCount = 3;
    StageTime = 0;
    
    title:
    Title();
    do {
        // if (ScanStop()) return;
        key = ScanKeys();
    } while ((key & (Keys_Button0 | Keys_Button1)) == 0);
    
    play:
    Score = 0;
    if ((key & Keys_Button1) == 0 && (key & Keys_Dir) == 0) {
        CurrentStage = 0;
    }
    RemainCount = 3;
    stage:
    InitStage();
    trying:
    InitTrying();
    Clock = 0;
    monsterNum = 0;
    timeDenom = MaxTimeDenom;
    Sound_Start();     
    StartBGM();
    do {
        tuh_task();
        //     if (ScanStop()) return;
        if ((Clock & 3) == 0) {
            WaitTimer(1);
            UpdateDestructions();
            FallMan();
            MoveMan();
            if (monsterNum >= 0) {
                FallMonsters();
                MoveMonsters();
                monsterNum -= 10;
            }
            monsterNum += 6;
            
            --timeDenom;
            if (timeDenom == 0) {
                --StageTime;
                timeDenom = MaxTimeDenom;
                PrintTime();
                if (StageTime == 0) {
                    PrintTimeUp();
                    lose:
                    StopBGM();
                    LooseMan();
                    --RemainCount;
                    if (RemainCount > 0) {
                        goto trying;
                    } 
                    PrintGameOver();
                    Sound_GameOver();
                    goto title;
                }
            }
        }
        // UpdateSprites();
        ++Clock;
        if ((Man.status & Movable_Live) == 0) {
            goto lose;
        }
    } while (ItemCount != 0);
    // ShowMan();
    // UpdateSprites();
    StopBGM();
    WaitTimer(10);
    Sound_Clear();
    while (StageTime >= BonusRate) {
        AddScore(5);
        StageTime -= BonusRate;
        PrintTime();
        Sound_Beep();
    }
    StageTime = 0;
    PrintStatus();
    ++CurrentStage;
    goto stage;
// exit:
//     Fin();
}


void AddScore(word pts) 
{
    Score += pts;
    if (Score > HiScore) {
        HiScore = Score;
    }
    PrintScore();
}

void _deb(){}