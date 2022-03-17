#ifndef _CHEATDRIVER_HOOKS_H_
#define _CHEATDRIVER_HOOKS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "ntifs.h"
#include "CheatDriver_Common.h"

#pragma pack ( push, 8 )
NTSTATUS
	Hooks_Startup();
NTSTATUS
	Hooks_Cleanup();

NTSTATUS
	FillHooksList(void* buffer, DWORD32 buffersize, PULONG pBytesWritten);
NTSTATUS
InsertHook(void* buffer, DWORD32 buffersize, PULONG pBytesWritten);

// TODO
void HookedIoctl();

KIRQL disableWP();

void enableWP(KIRQL	tempirql);

NTSTATUS SetHook(PCD_HOOK_STRUCT hookEntry);

#pragma pack ( pop )

#ifdef __cplusplus
};
#endif

#endif /* _CHEATDRIVER_HOOKS_H_ */
