#include "threading.h"

void threading::Sleep(int ms)
{
	//Negative values represent a relative interval
	LARGE_INTEGER lInt;
	int wait = ms >= 0 ? ms : MAXINT32;
	lInt.QuadPart = -10000 * (size_t)wait;
	do {
		KeDelayExecutionThread(KernelMode, FALSE, &lInt);
	} while (ms == SLEEP_FOREVER);
}