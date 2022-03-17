#include "CheatDriver.h"
#include "CheatDriver_Globals.h"
#include "CheatDriver_Hooks.h"
#include "CheatDriver_Modules.h"
#include "CheatDriver_Common.h"
#include <wdmsec.h>

extern "C"
{

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif

Globals_Struct Globals;

void DeleteAllDevices()
{
    PDEVICE_OBJECT      currentDevice;
    PDEVICE_OBJECT      previousDevice;

    currentDevice = Globals.DriverObject->DeviceObject;
    while (currentDevice != NULL)
    {
        KDebugLog("Removing Device at 0x%p\n", currentDevice);
        previousDevice = currentDevice;
        currentDevice = currentDevice->NextDevice;
        IoDeleteDevice(previousDevice);
    }

}

NTSTATUS RegisterDevice(const wchar_t* deviceName, const wchar_t* linkName)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UNICODE_STRING devName;
    UNICODE_STRING lnkName;
    PDEVICE_OBJECT deviceObject;

    if (wcslen(deviceName) >= 255
        || wcslen(linkName) >= 255)
    {
        return STATUS_INVALID_PARAMETER;
    }
    //TODO: string validation
    RtlInitUnicodeString(&devName, deviceName);
    RtlInitUnicodeString(&lnkName, linkName);

    // Create the device object and device extension
    ntStatus = IoCreateDeviceSecure(
        Globals.DriverObject,
        0,
        &devName,
        FILE_DEVICE_CHEATDRIVER,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &SDDL_DEVOBJ_SYS_ALL_ADM_ALL,
        &guidProto,
        &deviceObject);

    if (ntStatus == STATUS_SUCCESS)
    {
        // Create a symbolic link to allow USER applications to access it.
        ntStatus = IoCreateSymbolicLink(&lnkName, &devName);
        if (ntStatus == STATUS_SUCCESS)
        {
            // Tell the I/O Manger to do BUFFERED IO
            deviceObject->Flags |= DO_BUFFERED_IO;
            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            RtlZeroMemory(Globals.DeviceName.deviceName, sizeof(Globals.DeviceName.deviceName));
            RtlZeroMemory(Globals.DeviceName.symbolicName, sizeof(Globals.DeviceName.symbolicName));
            RtlCopyMemory(Globals.DeviceName.deviceName, deviceName, wcslen(deviceName) * sizeof(wchar_t));
            RtlCopyMemory(Globals.DeviceName.symbolicName, linkName, wcslen(linkName) * sizeof(wchar_t));
        }
        else
        {
            KDebugLog("IoCreateSymbolicLink Failed.  ntStatus= 0x%X\n", ntStatus);
            IoDeleteDevice(deviceObject);
        }
    }
    else
    {
        KDebugLog("IoCreateDevice Failed.  ntSTatus = 0x%X\n", ntStatus);
    }
    return ntStatus;
}

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    UNREFERENCED_PARAMETER(RegistryPath);
    BOOLEAN DeviceRegistered = FALSE;

    // Init Globals
    RtlZeroMemory(&Globals, sizeof(Globals));
    RtlCopyMemory(Globals.DeviceName.deviceName, CD_DEVICE_NAME, sizeof(CD_DEVICE_NAME));
    RtlCopyMemory(Globals.DeviceName.symbolicName, CD_SYMBOLIC_LINK_NAME, sizeof(CD_SYMBOLIC_LINK_NAME));
    RtlCopyMemory(Globals.DeviceName.win32LinkName, CD_WIN32_LINK_NAME, sizeof(CD_WIN32_LINK_NAME));
    
    Globals.DriverObject = DriverObject;
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpDeviceControl;

    KdPrint(("kdPrint Hello World\n"));
    KdPrint(("DeviceObject = 0x%p\n", DriverObject->DeviceObject));
    NtStatus = RegisterDevice(Globals.DeviceName.deviceName, Globals.DeviceName.symbolicName);
    if (NtStatus != STATUS_SUCCESS)
    {
        goto Cleanup;
    }
    DeviceRegistered = TRUE;
    NtStatus = Module_Startup();
    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }
    NtStatus = Hooks_Startup();
    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }
    KDebugLog("DeviceObject = 0x%p\n", DriverObject->DeviceObject);
Cleanup:
    if (!NT_SUCCESS(NtStatus))
    {
        // Do clean up
        if (TRUE == DeviceRegistered)
        {
            DeleteAllDevices();
        }
    }
    return NtStatus;
}

// Functions
// Lookup process by PID
// Get baseaddress of Driver
// Read
// Write
// Scan for pattern
// Place Hook

void
DriverUnload(
    IN     PDRIVER_OBJECT      DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNICODE_STRING      symbolicName;
    NTSTATUS ntStatus;
    LARGE_INTEGER wait;

    wait.HighPart = 0;
    wait.LowPart = 1000;
    RtlInitUnicodeString(&symbolicName, Globals.DeviceName.symbolicName);
    if (symbolicName.Length > 1)
    {
        KeDelayExecutionThread(KernelMode, false, &wait);
        KDebugLog("Deleting Symbolic Link: %wZ\n", symbolicName);
        ntStatus = IoDeleteSymbolicLink(&symbolicName);
        KDebugLog("DeleteSymbolicLink code = 0x%X\n", ntStatus);
    }
    // Clean up module list
    Module_Cleanup();
    Hooks_Cleanup();
    DeleteAllDevices();
    KdPrint(("YOU DIED!\n"));
}

NTSTATUS
IrpCreate(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN     PIRP                Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    // Ensure the device can only be opened once at a time
    if (Globals.HandleOpen)
    {
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
    }
    else
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Globals.HandleOpen = TRUE;
    }
    Irp->IoStatus.Information = 0;
    KdPrint(("IrpCreate\n"));
    //  Complete the request
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Irp->IoStatus.Status;
}

NTSTATUS
IrpClose(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN     PIRP                Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Globals.HandleOpen = FALSE;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    KdPrint(("IrpClose\n"));
    //  Complete the request
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
IrpDeviceControl(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION  IrpStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ULONG               DataReturned = 0;
    ULONG               IoctlCommand = 0;
    ULONG               OutLen = 0;
    ULONG               InLen = 0;
    PCHAR               Buffer = NULL;
    ULONG               nSize = 0;
    DWORD32             targetModule = 0;

    IoctlCommand = IrpStackLocation->Parameters.DeviceIoControl.IoControlCode;
    OutLen = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
    InLen = IrpStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    Buffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
    switch (IoctlCommand)
    {
    case IOCTL_CHEATDRIVER_READ:
        break;
    case IOCTL_CHEATDRIVER_WRITE:
        break;
    case IOCTL_CHEATDRIVER_SELECT:
        KdPrint(("IOCTL_CHEATDRIVER_SELECT\n"));
        targetModule = *(DWORD32*)(Buffer);
        NtStatus = SelectModule(targetModule, Buffer, OutLen, &DataReturned);
        break;
    case IOCTL_CHEATDRIVER_PLACEHOOK:
        if (NULL == Buffer)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        KdPrint(("IOCTL_CHEATDRIVER_PLACEHOOK\n"));
        NtStatus = InsertHook(Buffer, OutLen, &DataReturned);
        break;
    case IOCTL_CHEATDRIVER_LISTMODULES:
        if (NULL == Buffer)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        KdPrint(("IOCTL_CHEATDRIVER_LISTMODULES\n"));
        NtStatus = FillModuleListBuffer(Buffer, OutLen, &DataReturned);      
        break;
    case IOCTL_CHEATDRIVER_LISTHOOKS:
        if (NULL == Buffer)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        KdPrint(("IOCTL_CHEATDRIVER_LISTHOOKS\n"));
        NtStatus = FillHooksList(Buffer, OutLen, &DataReturned);
        break;
    default:
        NtStatus = STATUS_INVALID_PARAMETER;
        break;
    }

Cleanup:
    Irp->IoStatus.Status = NtStatus;
    if (STATUS_SUCCESS == NtStatus)
    {
        Irp->IoStatus.Information = DataReturned;
    }
    Irp->IoStatus.Information = DataReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return NtStatus;
}

} // End extern "C"