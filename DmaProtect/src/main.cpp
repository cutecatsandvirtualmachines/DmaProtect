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

    PVOID pBase = pDriverObj;

    PVOID pBuffer = cpp::kMalloc(PAGE_SIZE);
    RtlZeroMemory(pBuffer, PAGE_SIZE);

    DdmaProvider dma;
    dma.DiskCopyPage(pBuffer, pBase);

    PVOID pBufferPost = cpp::kMalloc(PAGE_SIZE);
    RtlZeroMemory(pBufferPost, PAGE_SIZE);
    dma.DiskCopyPage(pBufferPost, pBase);

    bool bEqual = memcmp(pBuffer, pBufferPost, PAGE_SIZE) == 0;

    DbgMsg("[RESULT] DMA remapping %s", bEqual ? "didn't work" : "worked");

    DbgMsg("[TEST] MZ check: %s", pBase);
    DbgMsg("[TEST] MZ check: %s", pBuffer);
    DbgMsg("[TEST] MZ check: %s", pBufferPost);

	return STATUS_SUCCESS;
}