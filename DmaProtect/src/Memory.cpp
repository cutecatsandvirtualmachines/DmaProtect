#include "Memory.h"

UINT64 Memory::VirtToPhy(PVOID Va)
{
	return MmGetPhysicalAddress(Va).QuadPart;
}

UINT64 Memory::PhyToVirt(UINT64 Pa)
{
	PHYSICAL_ADDRESS PhysicalAddr;
	PhysicalAddr.QuadPart = Pa;

	return (UINT64)MmGetVirtualForPhysical(PhysicalAddr);
}