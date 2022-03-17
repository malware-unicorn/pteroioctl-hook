#include "CheatDriver_Modules.h"
#include "CheatDriver_Common.h"
#include "CheatDriver_Globals.h"
#include <wdmsec.h>
#include <ntstrsafe.h>
#include <ntdef.h>
#include <sal.h>

#pragma data_seg("NONPAGE")

extern "C"
{

LIST_ENTRY gbModuleListHead;

NTSTATUS
Module_Startup()
{
	NTSTATUS Status = STATUS_SUCCESS;
	KeInitializeGuardedMutex(&(Globals.ModulesMutex));
	InitializeListHead(&gbModuleListHead);
	return Status;
}
NTSTATUS
Module_Cleanup()
{
	KeAcquireGuardedMutex(&(Globals.ModulesMutex));
	// Empty the list of protected processes
	while (!IsListEmpty(&gbModuleListHead))
	{
		PCD_MODULE_LIST pEntry = (PCD_MODULE_LIST)gbModuleListHead.Flink;
		// Remove the item from the books
		RemoveEntryList(&pEntry->ListEntry);
		// Free memory
		ExFreePoolWithTag(pEntry, 0x414141);
	}
	Globals.ModulesNum = 0;
	KeReleaseGuardedMutex(&(Globals.ModulesMutex));
	return STATUS_SUCCESS;
}


NTSTATUS FillModuleListBuffer(
	void* buffer, DWORD32 buffersize, PULONG pBytesWritten)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PCD_MODULE_STRUCT pModuleContent = NULL;
	PCD_MODULE_OUTPUT_STRUCT outputStruct = NULL;
	DWORD32 alertCount = 0;
	DWORD32 neededBufferSize = sizeof(CD_MODULE_OUTPUT_STRUCT);
	int i = 0;

	Status = FillModulesList();
	if (!NT_SUCCESS(Status))
	{
		goto Cleanup;
	}
	KeAcquireGuardedMutex(&(Globals.ModulesMutex));
	neededBufferSize = sizeof(CD_MODULE_OUTPUT_STRUCT) - sizeof(UCHAR)
		+ ((Globals.ModulesNum) * sizeof(CD_MODULE_STRUCT));

	if (neededBufferSize > buffersize)
	{
		KeReleaseGuardedMutex(&(Globals.ModulesMutex));
		*pBytesWritten = neededBufferSize;
		Status = STATUS_INVALID_BUFFER_SIZE;
		goto Cleanup;
	}
	outputStruct = (PCD_MODULE_OUTPUT_STRUCT)ExAllocatePoolWithTag(
		NonPagedPool, neededBufferSize, 0x414141);
	if (outputStruct == NULL)
	{
		KeReleaseGuardedMutex(&(Globals.ModulesMutex));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Cleanup;
	}
	RtlZeroMemory(outputStruct, neededBufferSize);
	outputStruct->ModuleListSizeInBytes = sizeof(CD_MODULE_STRUCT);
	outputStruct->NumberOfModules = Globals.ModulesNum;
	// Loop through list and append
	for (LIST_ENTRY* pEntry = gbModuleListHead.Flink; pEntry != &gbModuleListHead; pEntry = pEntry->Flink)
	{
		PCD_MODULE_LIST pCurrentEntry = (PCD_MODULE_LIST)pEntry;
		// KDebugLog("Baseaddr: %0x%X\n", (DWORD64)(pCurrentEntry->module.BaseAddr));
		// KDebugLog("Name: %s\n", pCurrentEntry->module.ModuleName);
		pModuleContent = (PCD_MODULE_STRUCT)(outputStruct->Modules + (i * outputStruct->ModuleListSizeInBytes));
		RtlCopyBytes(pModuleContent, &pCurrentEntry->module, sizeof(CD_MODULE_STRUCT));
		i++;
	}
	KeReleaseGuardedMutex(&(Globals.ModulesMutex));
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

NTSTATUS FillModulesList()
{
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG nSize = 0;
	PRTL_PROCESS_MODULES moduleArray;
	PCD_MODULE_LIST pCurrentHead = NULL;
	
	moduleArray = GetModuleList(&nSize);
	if (!moduleArray)
	{
		goto Cleanup;
	}
	// Clear the list
	Module_Cleanup();
	InitializeListHead(&gbModuleListHead);
	// Fill the list
	KeAcquireGuardedMutex(&(Globals.ModulesMutex));
	PRTL_PROCESS_MODULE_INFORMATION moduleInfo = moduleArray->Modules;
	for (int i = 0; i < moduleArray->NumberOfModules; ++i)
	{
		PCD_MODULE_LIST pNewEntry = NULL;
		pNewEntry = (PCD_MODULE_LIST)ExAllocatePoolWithTag(
			NonPagedPool, sizeof(CD_MODULE_LIST), 0x414141);
		if (NULL == pNewEntry)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			KeReleaseGuardedMutex(&(Globals.ModulesMutex));
			goto Cleanup;
		}
		PVOID kernelImageBase = moduleInfo[i].ImageBase;
		PCHAR kernelImage = (PCHAR)moduleInfo[i].FullPathName;
		
		RtlCopyMemory(pNewEntry->module.ModuleName, kernelImage, 256);
		pNewEntry->module.BaseAddr = (DWORD64)kernelImageBase;
		pNewEntry->module.ModuleID = (DWORD32)Globals.ModulesNum;
		Globals.ModulesNum++;
		InsertHeadList(&gbModuleListHead, &pNewEntry->ListEntry);
	}
	KeReleaseGuardedMutex(&(Globals.ModulesMutex));
Cleanup:
	if (moduleArray)
	{
		ExFreePoolWithTag(moduleArray, 0x414141);
	}
	return Status;
}


PRTL_PROCESS_MODULES GetModuleList(
	OUT PULONG nSize
)
{
    NTSTATUS status = STATUS_SUCCESS;
	PVOID buffer = NULL;
	UNICODE_STRING funcName;
	KdPrint(("Entering GetModuleList\n"));
	// Get ZwQuerySystemInformation Function
	RtlUnicodeStringInit(&funcName, L"ZwQuerySystemInformation");
	FUNC_ZWQUERYSYSTEMINFORMATION ZwQuerySystemInformation = (FUNC_ZWQUERYSYSTEMINFORMATION)MmGetSystemRoutineAddress(&funcName);
	if (NULL == ZwQuerySystemInformation)
	{
		status = STATUS_NOT_FOUND;
		goto Cleanup;
	}
	KdPrint(("Entering ZwQuerySystemInformation\n"));
	status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, nSize);
	if (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		buffer = ExAllocatePoolWithTag(NonPagedPool, *nSize, 0x414141);
		status = ZwQuerySystemInformation(SystemModuleInformation, buffer, *nSize, NULL);
		if (NT_SUCCESS(status))
		{
			return (PRTL_PROCESS_MODULES)buffer;
		}
	}
	
Cleanup:
	return (PRTL_PROCESS_MODULES)0;
}

NTSTATUS SelectModule(
	DWORD32 moduleID, void* buffer, DWORD32 buffersize, PULONG pBytesWritten)
{
	NTSTATUS Status = STATUS_SUCCESS;
	DWORD32 neededBufferSize = sizeof(CD_MODULE_STRUCT);
	KeAcquireGuardedMutex(&(Globals.ModulesMutex));
	for (LIST_ENTRY* pEntry = gbModuleListHead.Flink; pEntry != &gbModuleListHead; pEntry = pEntry->Flink)
	{
		PCD_MODULE_LIST pCurrentEntry = (PCD_MODULE_LIST)pEntry;
		if (pCurrentEntry->module.ModuleID == moduleID)
		{
			Globals.SelectedModule.BaseAddr = pCurrentEntry->module.BaseAddr;
			Globals.SelectedModule.ModuleID = pCurrentEntry->module.ModuleID;
			RtlCopyMemory(&(Globals.SelectedModule.ModuleName), pCurrentEntry->module.ModuleName, 256);
			Globals.SelectedModule.ModuleNameLen = pCurrentEntry->module.ModuleNameLen;
			break;
		}
	}
	KeReleaseGuardedMutex(&(Globals.ModulesMutex));
	RtlCopyBytes(buffer, &(Globals.SelectedModule), neededBufferSize);
	*pBytesWritten = neededBufferSize;
	return Status;
}


}