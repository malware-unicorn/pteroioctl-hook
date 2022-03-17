#include "CheatDriver_Hooks.h"
#include "CheatDriver_Common.h"
#include "CheatDriver_Globals.h"
#include <wdmsec.h>
#include <ntstrsafe.h>
#include <ntdef.h>
#include <sal.h>
#include <intrin.h>

#pragma data_seg("NONPAGE")

extern "C"
{
LIST_ENTRY gbHooksListHead;
UCHAR JUMP_RAX[] = "\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xE0";
UCHAR JUMP_WITH_R14[] = "\x56\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x8f\x00\x90\x90";


NTSTATUS
Hooks_Startup()
{
	NTSTATUS Status = STATUS_SUCCESS;
	KeInitializeGuardedMutex(&Globals.HooksMutex);
	InitializeListHead(&gbHooksListHead);
	return Status;
}

NTSTATUS 
Hooks_Cleanup()
{
	KeAcquireGuardedMutex(&(Globals.HooksMutex));
	// Empty the list of protected processes
	while (!IsListEmpty(&gbHooksListHead))
	{
		PCD_HOOK_LIST pEntry = (PCD_HOOK_LIST)gbHooksListHead.Flink;
		// Remove the item from the list
		RemoveEntryList(&pEntry->ListEntry);
		// Free memory
		ExFreePoolWithTag(pEntry, 0x414141);
	}
	Globals.HooksNum = 0;
	KeReleaseGuardedMutex(&(Globals.HooksMutex));
	return STATUS_SUCCESS;
}

NTSTATUS
FillHooksList(void* buffer, DWORD32 buffersize, PULONG pBytesWritten)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PCD_HOOK_STRUCT pHookContent = NULL;
	PCD_HOOKS_OUTPUT_STRUCT outputStruct = NULL;
	DWORD32 neededBufferSize = sizeof(CD_HOOKS_OUTPUT_STRUCT);
	int i = 0;

	KeAcquireGuardedMutex(&(Globals.HooksMutex));
	neededBufferSize = sizeof(CD_HOOKS_OUTPUT_STRUCT) - sizeof(UCHAR)
		+ ((Globals.HooksNum) * sizeof(CD_HOOK_STRUCT));
	if (neededBufferSize > buffersize)
	{
		KeReleaseGuardedMutex(&(Globals.HooksMutex));
		*pBytesWritten = neededBufferSize;
		Status = STATUS_INVALID_BUFFER_SIZE;
		goto Cleanup;
	}
	outputStruct = (PCD_HOOKS_OUTPUT_STRUCT)ExAllocatePoolWithTag(
		NonPagedPool, neededBufferSize, 0x414141);
	if (outputStruct == NULL)
	{
		KeReleaseGuardedMutex(&(Globals.HooksMutex));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Cleanup;
	}
	RtlZeroMemory(outputStruct, neededBufferSize);
	outputStruct->HooksListSizeInBytes = sizeof(CD_HOOK_STRUCT);
	outputStruct->NumberOfHooks = Globals.HooksNum;
	// Loop through list and append
	
	for (LIST_ENTRY* pEntry = gbHooksListHead.Flink; pEntry != &gbHooksListHead; pEntry = pEntry->Flink)
	{
		PCD_HOOK_LIST pCurrentEntry = (PCD_HOOK_LIST)pEntry;
		pHookContent = (PCD_HOOK_STRUCT)(outputStruct->Hooks + (i * outputStruct->HooksListSizeInBytes));
		RtlCopyBytes(pHookContent, &pCurrentEntry->HookInfo, sizeof(CD_HOOK_STRUCT));
		i++;
	}
	KeReleaseGuardedMutex(&(Globals.HooksMutex));
	RtlCopyBytes(buffer, outputStruct, neededBufferSize);
	*pBytesWritten = neededBufferSize;
	Status = STATUS_SUCCESS;
Cleanup:
	if (NULL != outputStruct)
	{
		ExFreePoolWithTag(outputStruct, 0x414141);
	}
	return Status;
}

NTSTATUS
InsertHook(void* buffer, DWORD32 buffersize, PULONG pBytesWritten)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PCD_HOOK_LIST pNewEntry = NULL;
	PCD_HOOK_STRUCT pUserHookContent = NULL;
	if (buffersize < sizeof(CD_HOOK_STRUCT))
	{
		Status = STATUS_INVALID_PARAMETER;
		goto Cleanup;
	}
	pNewEntry = (PCD_HOOK_LIST)ExAllocatePoolWithTag(
		NonPagedPool, sizeof(CD_HOOK_LIST), 0x414141);
	if (NULL == pNewEntry)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Cleanup;
	}
	RtlCopyMemory(&pNewEntry->HookInfo, buffer, sizeof(CD_HOOK_STRUCT));
	pNewEntry->HookInfo.ModuleID = Globals.SelectedModule.ModuleID;
	KeAcquireGuardedMutex(&(Globals.HooksMutex));
	Globals.HooksNum++;
	// TODO: Call to Set Hook
	// SetHook(&(pNewEntry->HookInfo));
	InsertHeadList(&gbHooksListHead, &pNewEntry->ListEntry);
	KeReleaseGuardedMutex(&(Globals.HooksMutex));
	
Cleanup:
	return Status;
}

NTSTATUS SetHook(PCD_HOOK_STRUCT hookEntry)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PVOID64 OriginalAddress = (PVOID64)(hookEntry->Hook.EntryAddr);
	PVOID64 JmpToAddress = (PVOID64)(hookEntry->Hook.JumpAddr); // Destination
	DWORD32 AddrOffset = hookEntry->Hook.CustomHookAddrOffset;
	PUCHAR HookBuffer = hookEntry->Hook.CustomHookBuffer;
	DWORD32 HookLen = hookEntry->Hook.CustomHookLen;

	if (NULL == hookEntry)
	{
		Status = STATUS_INVALID_PARAMETER;
		goto Cleanup;
	}
	KIRQL tempIRQL = disableWP();
	RtlCopyMemory(HookBuffer + AddrOffset, &JmpToAddress, sizeof(PVOID64));
	RtlCopyMemory((PVOID64)OriginalAddress, HookBuffer, HookLen);
	enableWP(tempIRQL);
Cleanup:
	return Status;
}

KIRQL disableWP()
{
	KIRQL tempirql = KeRaiseIrqlToDpcLevel();
	ULONG64 cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	_disable();
	return tempirql;
}

void enableWP(KIRQL	tempirql)
{
	ULONG64	cr0 = __readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(tempirql);
}

#pragma optimize("", off)
void HookedIoctl()
{

	int a1 = 1;
	int a2 = 1;
	int a3 = 1;		// 32 free bytes
	int a4 = 1;
	// DOSTUFF();
	int a5 = 1;
	int a6 = 1;		// 32 free bytes
	int a7 = 1;
	int a8 = 1;
}
#pragma optimize("", on)


}