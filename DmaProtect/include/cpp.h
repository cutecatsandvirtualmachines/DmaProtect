#pragma once
#include <ntddk.h>

namespace cpp {
	void* kMalloc(size_t sz);
	void kFree(void* pBuffer);
}