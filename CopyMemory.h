#pragma once
#include <string.h>

inline void CopyMemory(ptr<byte> pDestination, constptr<byte> pSource, word length)
{
    memcpy(pDestination, pSource, length);
}

// extern void FillMemory(ptr<byte> pDestination, word length, byte b);
