#include "cpp.h"

void* cpp::kMalloc(size_t sz)
{
	return ExAllocatePool(POOL_TYPE::NonPagedPool, sz);
}

void cpp::kFree(void* pBuffer)
{
	ExFreePool(pBuffer);
}
