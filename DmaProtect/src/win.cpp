#include "win.h"

void* win::FindCurrentBase()
{
    void* currPtr = PAGE_ALIGN(FindCurrentBase);

    while (*(SHORT*)currPtr != (SHORT)'MZ') {
        currPtr = (PVOID)((DWORD64)currPtr - PAGE_SIZE);
    }
    return currPtr;
}
