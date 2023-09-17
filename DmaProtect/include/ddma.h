#pragma once

#include "cpp.h"

#ifdef _KERNEL_MODE

typedef struct _DISK {
    PDEVICE_OBJECT Device;

    // Buffer holding sectors original data
    UINT8 Buffer[PAGE_SIZE];
} DISK, * PDISK;

class DdmaProvider {
private:
    PDISK pDisk;

public:
    DdmaProvider();
    ~DdmaProvider();

    NTSTATUS DiskCopy(PVOID dest, PVOID src);
};

#endif