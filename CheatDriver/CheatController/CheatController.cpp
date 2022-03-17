// CheatController.cpp : This file contains the 'main' function. Program execution begins and ends there.
// certmgr /add CheatDriver.cer /s /r localMachine root
//bcdedit / set testsigning on

#include <stdio.h>
#include <StrSafe.h>
#include <Windows.h>
#include <tchar.h>
#include <ntstatus.h>
#include "../CheatDriver/CheatDriver_Common.h"

#define LOG_ERROR(FormatString, ...)
#define LOG_INFO(FormatString, ...) 
#define LOG_INFO_FAILURE(FormatString, ...)
#define LOG_PASSED(FormatString, ...)

WCHAR DriverPath[MAX_PATH];
SC_HANDLE TcScmHandle = NULL;
UCHAR JUMP_RAX[] = "\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xE0";
UCHAR JUMP_WITH_R14[] = "\x56\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x8f\x00\x90\x90";

void PrintHeader()
{
	printf("                           <\\              _\n"
		"                            \\\\          _ / {\n"
		"                     _       \\\\       _-   -_\n"
		"                   /{        / `\\   _-     - -_\n"
		"                 _~  =      ( @  \\ -        -  -_\n"
		"               _- -   ~-_   \\( =\\ \\           -  -_\n"
		"             _~  -       ~_ | 1 :\\ \\      _-~-_ -  -_\n"
		"           _-   -          ~  |V: \\ \\  _-~     ~-_-  -_\n"
		"        _-~   -            /  | :  \\ \\            ~-_- -_\n"
		"     _-~    -   _.._      {   | : _-``               ~- _-_\n"
		"  _-~   -__..--~    ~-_  {   : \\:}\n"
		"=~__.--~~              ~-_\\  :  /\n"
		"                           \\ : /__\n"
		"                          //`Y'--\\\\      = IOCTL\n"
		"                         <+       \\\\\n"
		"                          \\\\      WWW\n"
		"                          MMM\n");
}

BOOL TcGetServiceState(
	_In_ SC_HANDLE ServiceHandle,
	_Out_ DWORD* State
)
{
	SERVICE_STATUS_PROCESS ServiceStatus;
	DWORD BytesNeeded;

	*State = 0;

	BOOL Result = QueryServiceStatusEx(
		ServiceHandle,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ServiceStatus,
		sizeof(ServiceStatus),
		&BytesNeeded
	);

	if (Result == FALSE)
	{
		printf("TcGetServiceState: QueryServiceStatusEx failed, last error 0x%x\n", GetLastError());
		return FALSE;
	}

	*State = ServiceStatus.dwCurrentState;

	return TRUE;
}

BOOL TcWaitForServiceState(
	_In_ SC_HANDLE ServiceHandle,
	_In_ DWORD State
)
{
	for (;;)
	{
		//printf("TcWaitForServiceState: Waiting for service %p to enter state %u...\n", (DWORD_PTR)ServiceHandle, State);

		DWORD ServiceState;
		BOOL Result = TcGetServiceState(ServiceHandle, &ServiceState);

		if (Result == FALSE)
		{
			printf("TcWaitForServiceState: failed to get state\n");
			return FALSE;
		}

		if (ServiceState == State)
		{
			break;
		}

		Sleep(1000);
	}
	//printf("TcWaitForServiceState: Success service %p in state %u...\n", (DWORD_PTR)ServiceHandle, State);
	return TRUE;
}

BOOL TcDeleteService()
{
	BOOL ReturnValue = FALSE;


	LOG_INFO(L"TcDeleteService: Entering\n");

	//
	// Open the service so we can delete it
	//

	SC_HANDLE ServiceHandle = OpenService(
		TcScmHandle,
		(LPCWSTR)CD_DRIVER_NAME,
		SERVICE_ALL_ACCESS
	);

	DWORD LastError = GetLastError();

	if (ServiceHandle == NULL)
	{
		if (LastError == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			ReturnValue = TRUE;
		}
		else
		{
			LOG_INFO_FAILURE(L"TcDeleteService: OpenService failed, last error 0x%x\n", LastError);
		}

		goto Cleanup;
	}

	//
	// Delete the service
	//

	if (!DeleteService(ServiceHandle))
	{
		LastError = GetLastError();

		if (LastError != ERROR_SERVICE_MARKED_FOR_DELETE)
		{
			LOG_INFO_FAILURE(L"TcDeleteService: DeleteService failed, last error 0x%x", LastError);
			goto Cleanup;
		}
	}

	ReturnValue = TRUE;

Cleanup:

	if (ServiceHandle)
	{
		CloseServiceHandle(ServiceHandle);
	}

	LOG_INFO(L"TcDeleteService: Exiting");

	return ReturnValue;
}

BOOL TcCreateService()
{
	BOOL ReturnValue = FALSE;

	printf("TcCreateService: Entering\n");

	//
	// Create the service
	//
	printf("Trying to install the service %ws\n", DriverPath);

	SC_HANDLE ServiceHandle = CreateServiceW(
		TcScmHandle,            // handle to SC manager
		CD_DRIVER_NAME,         // name of service
		CD_DRIVER_NAME,         // display name
		SERVICE_ALL_ACCESS,    // access mask
		SERVICE_KERNEL_DRIVER,  // service type
		SERVICE_DEMAND_START,   // start type
		SERVICE_ERROR_NORMAL,   // error control
		DriverPath,           // full path to driver
		NULL,                   // load ordering
		NULL,                   // tag id
		NULL,                   // dependency
		NULL,                   // account name
		NULL                    // password
	);

	DWORD LastError = GetLastError();

	if (ServiceHandle == NULL && LastError != ERROR_SERVICE_EXISTS)
	{
		printf("CreateService failed, last error 0x%x\n", LastError);
		goto Cleanup;
	}

	ReturnValue = TRUE;

Cleanup:

	if (ServiceHandle)
	{
		CloseServiceHandle(ServiceHandle);
	}

	printf("TcCreateService: Exiting\n");

	return ReturnValue;
}

BOOL TcStopService()
{
	BOOL ReturnValue = FALSE;

	printf("TcStopService: Entering\n");

	//
	// Open the service so we can stop it
	//

	SC_HANDLE ServiceHandle = OpenService(
		TcScmHandle,
		(LPCWSTR)CD_DRIVER_NAME,
		SERVICE_ALL_ACCESS
	);

	DWORD LastError = GetLastError();

	if (ServiceHandle == NULL)
	{
		if (LastError == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			ReturnValue = TRUE;
		}
		else
		{
			printf("TcStopService: OpenService failed, last error 0x%x\n", LastError);
		}

		goto Cleanup;
	}

	//
	// Stop the service
	//

	SERVICE_STATUS ServiceStatus;

	if (FALSE == ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus))
	{
		LastError = GetLastError();

		if (LastError != ERROR_SERVICE_NOT_ACTIVE)
		{
			printf("TcStopService: ControlService failed, last error 0x%x\n", LastError);
			goto Cleanup;
		}
	}

	if (FALSE == TcWaitForServiceState(ServiceHandle, SERVICE_STOPPED))
	{
		goto Cleanup;
	}

	ReturnValue = TRUE;

Cleanup:

	if (ServiceHandle)
	{
		CloseServiceHandle(ServiceHandle);
	}

	printf("TcStopService: Exiting\n");

	return ReturnValue;
}

BOOL TcStartService()
{
	BOOL ReturnValue = FALSE;
	printf("TcStartService: Entering\n");
	//
	// Open the service. The function assumes that
	// TdCreateService has been called before this one
	// and the service is already installed.
	//

	SC_HANDLE ServiceHandle = OpenServiceW(
		TcScmHandle,
		CD_DRIVER_NAME,
		SERVICE_ALL_ACCESS
	);

	if (ServiceHandle == NULL)
	{
		printf("TcStartService: OpenService failed, last error 0x%x\n", GetLastError());
		goto Cleanup;
	}

	//
	// Start the service
	//

	if (!StartService(ServiceHandle, 0, NULL))
	{
		if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("TcStartService: StartService failed, last error 0x%x\n", GetLastError());
			goto Cleanup;
		}
	}

	if (FALSE == TcWaitForServiceState(ServiceHandle, SERVICE_RUNNING))
	{
		goto Cleanup;
	}

	ReturnValue = TRUE;

Cleanup:

	if (ServiceHandle)
	{
		CloseServiceHandle(ServiceHandle);
	}

	return ReturnValue;
}

BOOL TcUnloadDriver()
{
	BOOL ReturnValue = FALSE;

	printf("TcUnloadDriver: Entering\n");


	//
	// Unload the driver.
	//

	ReturnValue = TcStopService();

	if (ReturnValue == FALSE)
	{
		printf("TcStopService failed\n");
		goto Cleanup;
	}

	//
	// Delete the service.
	//

	ReturnValue = TcDeleteService();

	if (ReturnValue == FALSE)
	{
		printf("TcDeleteService failed\n");
		goto Cleanup;
	}

	ReturnValue = TRUE;

Cleanup:

	printf("TcUnloadDriver: Exiting\n");

	return ReturnValue;
}

BOOL TcLoadDriver()
{
	BOOL ReturnValue = FALSE;

	printf("TcLoadDriver: Entering\n");

	//
	// First, uninstall and unload the driver. 
	//

	ReturnValue = TcUnloadDriver();

	if (ReturnValue != TRUE)
	{
		printf("TcUnloadDriver failed\n");
		goto Cleanup;
	}

	ReturnValue = TcCreateService();

	if (ReturnValue == FALSE)
	{
		printf("TcCreateService failed\n");
		goto Cleanup;
	}

	//
	// Load the driver.
	//

	ReturnValue = TcStartService();

	if (ReturnValue == FALSE)
	{
		printf("TcStartService failed\n");
		goto Cleanup;
	}


	ReturnValue = TRUE;

Cleanup:

	printf("TcLoadDriver: Exiting\n");
	return ReturnValue;
}

BOOL TcInstallDriver()
{
	BOOL bRC = TRUE;

	printf("TcInstallDriver: Entering\n");
	BOOL Result = TcLoadDriver();

	if (Result != TRUE)
	{
		LOG_ERROR(L"TcLoadDriver failed, exiting\n");
		bRC = FALSE;
		goto Cleanup;
	}

Cleanup:

	printf("TcInstallDriver: Exiting\n");
	return bRC;
}

BOOL Init() {
	BOOL ReturnValue = FALSE;
	// Create Serivice
	if (TcScmHandle == NULL)
	{
		TcScmHandle = OpenSCManager(
			NULL,
			NULL,
			SC_MANAGER_ALL_ACCESS
		);

		if (TcScmHandle == NULL)
		{
			printf("OpenSCManager failed, last error 0x%x\n", GetLastError());
			goto Cleanup;
		}
	}

	// Get Driver Path
	GetCurrentDirectoryW(ARRAYSIZE(DriverPath), DriverPath);
	wcsncat_s(DriverPath, L"\\CheatDriver.sys", 
		ARRAYSIZE(DriverPath) - wcsnlen_s(DriverPath, ARRAYSIZE(DriverPath)));
	printf("DriverPath %ws\n", DriverPath);
	ReturnValue = TRUE;

Cleanup:
	return ReturnValue;	
}

int wmain(
	_In_ int argc,
	_In_ LPCWSTR argv[]
)
{
	HANDLE    deviceHandle;
	ULONG     status;
	ULONG     bytesReturned;
	CHAR      input[MAX_PATH];
	PVOID     outputBuffer;
	ULONG     outputBufferSize = 0x10000;
	HANDLE    procHandle;
	DEVICE_NAME_STRUCT newName = { };
	PCD_MODULE_OUTPUT_STRUCT pModuleOutput;
	PCD_MODULE_STRUCT ModuleInfo;
	PCD_HOOKS_OUTPUT_STRUCT pHookOutput;
	PCD_HOOK_STRUCT HookInfo;
	CD_HOOK_STRUCT newHook = { };
	DWORD retVal = 0;
	memcpy(newName.deviceName, CD_DEVICE_NAME, wcslen(CD_DEVICE_NAME) * sizeof(wchar_t));
	memcpy(newName.symbolicName, CD_SYMBOLIC_LINK_NAME, wcslen(CD_SYMBOLIC_LINK_NAME) * sizeof(wchar_t));

	// allocate a big output buffer...this is a shortcut but tell nobody...
	outputBuffer = malloc(outputBufferSize);
	if (outputBuffer == NULL)
	{
		return -1;
	}

	if (argc > 0)
	{
		const wchar_t* arg = argv[1];
		if (!Init())
		{
			printf("Init Fail!\n");
			return 1;
		}
		if (0 == wcscmp(arg, L"-load"))
		{
			PrintHeader();
			printf("Loading driver ...\n");
			TcInstallDriver();
		}
	}
	// Open the target device
	deviceHandle = CreateFileW(CD_WIN32_LINK_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		0);

	if (deviceHandle == INVALID_HANDLE_VALUE)
	{
		status = GetLastError();
		printf("CreateFile Error 0x%x\n", status);
		return 1;
	}
	printf("CreateFile 0x%x\n", deviceHandle);
	while (1)
	{
		memset(outputBuffer, 0, outputBufferSize);
		memset(input, 0, MAX_PATH);
		DWORD32 moduleid = 0;

		printf("\n>>>> Cheat Console:\n\n");
		printf("\t1                List Modules\n");
		printf("\t2                Select Module\n");
		printf("\t3                List Hooks\n");
		printf("\t4                Place Hook (Inserts Into list, TODO: set hook)\n");
		printf("\t5                Kernel Memory Read (TODO)\n");
		printf("\t6                Kernel Memory Write (TODO)\n");
		printf("\t7                Get Trampoline Func Address (TODO)\n");
		printf("\n\t0                Exit\n");
		printf("\n\tSelection: ");

		StringCchGetsA(input, MAX_PATH - 1);

		switch (input[0])
		{
		case '0':
		{
			CloseHandle(deviceHandle);
			return 0;
		}
		case '1':
		{
			RtlZeroMemory(outputBuffer, outputBufferSize);
			status = DeviceIoControl(deviceHandle,
				(DWORD)IOCTL_CHEATDRIVER_LISTMODULES,
				NULL,
				0,
				outputBuffer,
				outputBufferSize,
				&bytesReturned,
				0);

			printf("Bytes sent from Kernel: 0x%X with status 0x%X\n", bytesReturned, status);
			pModuleOutput = (PCD_MODULE_OUTPUT_STRUCT)outputBuffer;
			printf("Number of modules: 0x%X, Bytes: 0x%X\n", pModuleOutput->NumberOfModules, pModuleOutput->ModuleListSizeInBytes);
			for (unsigned int i = 0; i < pModuleOutput->NumberOfModules; i++)
			{
				ModuleInfo = (PCD_MODULE_STRUCT)(pModuleOutput->Modules + (i * pModuleOutput->ModuleListSizeInBytes));
				printf("|%d\t|\t0x%X\t|\t%s\n", ModuleInfo->ModuleID, ModuleInfo->BaseAddr, ModuleInfo->ModuleName);
			}
			break;
		}
		case '2':
		{
			printf("\n\tEnter Module ID: ");
			StringCchGetsA(input, MAX_PATH - 1);
			DWORD32 moduleSelection = 0;
			int nSize = sscanf_s(input, "%d", &moduleSelection);
			printf("\n\tSelected %d", moduleSelection);
			memcpy(outputBuffer, &moduleSelection, sizeof(DWORD32));
			status = DeviceIoControl(deviceHandle,
				(DWORD)IOCTL_CHEATDRIVER_SELECT,
				outputBuffer,
				sizeof(DWORD32),
				outputBuffer,
				outputBufferSize,
				&bytesReturned,
				0);

			printf("Bytes sent from Kernel: 0x%X with status 0x%X\n", bytesReturned, status);
			ModuleInfo = (PCD_MODULE_STRUCT)outputBuffer;
			printf("SELECTED: |%d\t|\t0x%X\t|\t%s\n", ModuleInfo->ModuleID, ModuleInfo->BaseAddr, ModuleInfo->ModuleName);
			break;
		}
		case '3':
		{
			RtlZeroMemory(outputBuffer, outputBufferSize);
			status = DeviceIoControl(deviceHandle,
				(DWORD)IOCTL_CHEATDRIVER_LISTHOOKS,
				NULL,
				0,
				outputBuffer,
				outputBufferSize,
				&bytesReturned,
				0);

			printf("Bytes sent from Kernel: 0x%X with status 0x%X\n", bytesReturned, status);
			pHookOutput = (PCD_HOOKS_OUTPUT_STRUCT)outputBuffer;
			printf("Number of hooks: 0x%X, Bytes: 0x%X\n", pHookOutput->NumberOfHooks, pHookOutput->HooksListSizeInBytes);
			for (unsigned int i = 0; i < pHookOutput->NumberOfHooks; i++)
			{
				HookInfo = (PCD_HOOK_STRUCT)(pHookOutput->Hooks + (i * pHookOutput->HooksListSizeInBytes));
				printf("|%d\t|\t0x%X\t|\t%d\n", 
					HookInfo->ModuleID, HookInfo->Hook.EntryAddr, HookInfo->Hook.HookStatus);
			}
			break;
		}
		case '4':
		{
			// This is a test
			newHook.Hook.EntryAddr = 0x1234;
			newHook.Hook.JumpAddr = 0x4567;
			newHook.Hook.ReturnAddr = 0x9123;
			newHook.Hook.IsCustomHook = TRUE;
			memcpy(newHook.Hook.CustomHookBuffer, JUMP_RAX, sizeof(JUMP_RAX));
			newHook.Hook.CustomHookLen = sizeof(JUMP_RAX);

			memcpy(outputBuffer, &newHook, sizeof(newHook));
			status = DeviceIoControl(deviceHandle,
				(DWORD)IOCTL_CHEATDRIVER_PLACEHOOK,
				outputBuffer,
				sizeof(newHook),
				outputBuffer,
				outputBufferSize,
				&bytesReturned,
				0);
			if (status == STATUS_SUCCESS)
			{
				printf("Handle successfully transfered.");
			}
			break;
		}
		case '5':
		{
			// TODO
			break;
		}
		case '6':
		{
			// TODO
			break;
		}
		
		}
	}

}