#pragma once
#include "cate.h"

extern word Score;
extern word HiScore;
extern byte RemainCount;
extern byte CurrentStage;
extern byte StageTime;
extern byte ItemCount;

extern void Main();
extern void AddScore(word pts);
extern void WaitTimer(byte t);
