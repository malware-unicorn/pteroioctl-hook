#pragma once


#pragma pack ( push, 8 )
#ifdef __cplusplus
extern "C"
{
#endif

#include "ntifs.h"
#include "CheatDriver_Common.h"

#ifdef DBG
#define KDebugLog( fmt, ... ) \
        DbgPrint("%s:%u: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define KDebugLog( fmt, ... ) 
#endif

typedef struct _Globals 
{
	BOOLEAN HandleOpen;
	PDRIVER_OBJECT DriverObject;
	DEVICE_NAME_STRUCT DeviceName;
	KGUARDED_MUTEX ModulesMutex;
	KGUARDED_MUTEX HooksMutex;
	CD_MODULE_STRUCT SelectedModule;
	ULONG ModulesNum;
	ULONG HooksNum;
} Globals_Struct, * pGlobals_Struct;

extern Globals_Struct Globals;

#ifdef __cplusplus
};
#endif
#pragma pack ( pop )
