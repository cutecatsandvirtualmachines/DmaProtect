#include "cpu.h"

bool CPU::bIntelCPU = false;

bool CPU::IsIntelCPU()
{
    bool isIntel = Cpuid::Cpuid::query<Cpuid::Generic::MaximumFunctionNumberAndVendorId>()->isIntel();
    if (isIntel)
        DbgMsg("[CPU] Is Intel");
    else
        DbgMsg("[CPU] Is not Intel");
    return isIntel;
}

void CPU::Init()
{
    bIntelCPU = IsIntelCPU();
}

VOID* CPU::WriteBackDataCacheRange(VOID* Address, SIZE_T Length)
{
    int  CpuIdData[4];
    SIZE_T   CacheLineSize;
    SIZE_T   Start;
    SIZE_T   End;

    if (Length == 0) {
        return Address;
    }

    //
    // If the CPU does not support CLFLUSH instruction,
    // then promote flush range to flush entire cache.
    //
    __cpuid(CpuIdData, 0x1);
    if ((CpuIdData[4] & (1ull << 19)) == 0) {
        __wbinvd();
        return Address;
    }

    //
    // Cache line size is 8 * Bits 15-08 of EBX returned from CPUID 01H
    //
    CacheLineSize = (CpuIdData[2] & 0xff00) >> 5;

    Start = (SIZE_T)Address;
    //
    // Calculate the cache line alignment
    //
    End = (Start + Length + (CacheLineSize - 1)) & ~(CacheLineSize - 1);
    Start &= ~((SIZE_T)CacheLineSize - 1);

    while (Start < End) {
        _mm_clflush((VOID*)Start);
        Start = Start + CacheLineSize;
    }

    return Address;
}
