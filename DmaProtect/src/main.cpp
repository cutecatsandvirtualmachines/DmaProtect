#include "iommu.h"
#include "ddma.h"
#include "status.h"
#include "win.h"
#include "pe.h"

PIMAGE_NT_HEADERS64 GetNtHeaders(void* image_base) {
    const auto dos_header = (PIMAGE_DOS_HEADER)(image_base);
    const auto nt_headers = (PIMAGE_NT_HEADERS64)((DWORD64)(image_base)+dos_header->e_lfanew);
    return nt_headers;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryPath) {
    UNREFERENCED_PARAMETER(pRegistryPath);

	CPU::Init();

    if (!iommu::Init()) {
        DbgMsg("[DMA] Failed initializing DMA protection!");
        return STATUS_DMA_REMAPPING_NOT_AVAILABLE;
    }

    PVOID pBase = pDriverObj;
    PIMAGE_NT_HEADERS64 pNtHeader = GetNtHeaders(pBase);

    PVOID pBuffer = cpp::kMalloc(PAGE_SIZE);
    RtlZeroMemory(pBuffer, PAGE_SIZE);

    DdmaProvider dma;
    dma.DiskCopy(pBuffer, pBase);

    if (!iommu::HidePageRange(pBase, pNtHeader->OptionalHeader.SizeOfImage)) {
        DbgMsg("[DMA] Failed protecting from DMA accesses!");
        return STATUS_DMA_REMAPPING_FAILED;
    }
    iommu::EnableIommu();

    PVOID pBufferPost = cpp::kMalloc(PAGE_SIZE);
    RtlZeroMemory(pBufferPost, PAGE_SIZE);
    dma.DiskCopy(pBufferPost, pBase);

    bool bEqual = memcmp(pBuffer, pBufferPost, PAGE_SIZE) == 0;

    DbgMsg("[RESULT] DMA remapping %s", bEqual ? "didn't work" : "worked");

    DbgMsg("[TEST] MZ check: %s", pBuffer);
    DbgMsg("[TEST] MZ check: %s", pBufferPost);

	return STATUS_SUCCESS;
}