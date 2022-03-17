#ifndef _CHEATDRIVER_H_
#define _CHEATDRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "ntifs.h"

#pragma pack ( push, 8 )


NTSTATUS
DriverEntry(
        IN PDRIVER_OBJECT DriverObject,
        IN PUNICODE_STRING RegistryPath);

void
DriverUnload(
        IN PDRIVER_OBJECT DriverObject );

NTSTATUS
IrpCreate(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN     PIRP                Irp);

NTSTATUS
IrpClose(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN     PIRP                Irp);

NTSTATUS
IrpDeviceControl(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp);

#pragma pack ( pop )

#ifdef __cplusplus
};
#endif

#endif /* _CHEATDRIVER_H_ */
