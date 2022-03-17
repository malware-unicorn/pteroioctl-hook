#ifndef _CHEATDRIVER_COMMON_H_
#define _CHEATDRIVER_COMMON_H_

#pragma pack ( push, 8 )

#define FILE_DEVICE_CHEATDRIVER      0x00001337
#define CD_SYMBOLIC_LINK_NAME          L"\\DosDevices\\Global\\CheatDriver"
#define CD_DEVICE_NAME                 L"\\Device\\CheatDriver"
#define CD_WIN32_LINK_NAME             L"\\??\\CheatDriver"
#define CD_DRIVER_NAME                 L"CheatDriver"

//{88BAE032-5A81-49f0-BC3D-A4FF138216D6}
//static const GUID guidProto = { 0x88BAE032, 0x5A81, 0x49f0, { 0xBC, 0x3D, 0xA4, 0xFF, 0x13, 0x82, 0x16, 0xD6 } };
static const GUID guidProto = { 0xb1133a4, 0xf98e, 0x4029, { 0xac, 0x56, 0xd, 0x1f, 0x35, 0xe, 0xa0, 0xda } };

// User requests to read at a specific address of N size chunked buffer
#define IOCTL_CHEATDRIVER_READ \
        (ULONG) CTL_CODE(FILE_DEVICE_CHEATDRIVER, 0x101, METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)
// User sends a buffer to be written at a specific address
// of N size chunked buffer
#define IOCTL_CHEATDRIVER_WRITE \
        (ULONG) CTL_CODE(FILE_DEVICE_CHEATDRIVER, 0x102, METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)
// User selects module to add from list of loaded modules
#define IOCTL_CHEATDRIVER_SELECT \
        (ULONG) CTL_CODE(FILE_DEVICE_CHEATDRIVER, 0x103, METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)
// User sends driver a hook to place
#define IOCTL_CHEATDRIVER_PLACEHOOK \
        (ULONG) CTL_CODE(FILE_DEVICE_CHEATDRIVER, 0x104, METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)
// User asks to list current loaded modules
#define IOCTL_CHEATDRIVER_LISTMODULES \
        (ULONG) CTL_CODE(FILE_DEVICE_CHEATDRIVER, 0x105, METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)
// User asks to list current module hooks
#define IOCTL_CHEATDRIVER_LISTHOOKS \
        (ULONG) CTL_CODE(FILE_DEVICE_CHEATDRIVER, 0x106, METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)

typedef struct _DEVICE_NAME_STRUCT {
    wchar_t symbolicName[255];
    wchar_t deviceName[255];
    wchar_t win32LinkName[255];
}DEVICE_NAME_STRUCT, * PDEVICE_NAME_STRUCT;

typedef enum
{
    CD_Hook_None = 0,
    CD_Hook_Success = 1,
    CD_Hook_Fail = 2
} CdHookStatus;

typedef struct _CD_HOOK_OFFSET_STRUCT {
    DWORD64 EntryAddr; // start of func
    DWORD64 ReturnAddr; // return address
    DWORD64 JumpAddr;
    CdHookStatus HookStatus; // if established
    BOOLEAN IsCustomHook;
    UCHAR CustomHookBuffer[256]; // Assembly data to write
    DWORD32 CustomHookLen;
    DWORD32 CustomHookAddrOffset; // Offset of custom hookbuffer
} CD_HOOK_OFFSET_STRUCT, * PCD_HOOK_OFFSET_STRUCT;

typedef struct _CD_HOOK_STRUCT {
    DWORD32            ModuleID;
    CD_HOOK_OFFSET_STRUCT Hook;
} CD_HOOK_STRUCT, * PCD_HOOK_STRUCT;

// List of hooks defined by user
typedef struct _CD_HOOK_LIST {
    LIST_ENTRY          ListEntry;
    CD_HOOK_STRUCT      HookInfo;
} CD_HOOK_LIST, * PCD_HOOK_LIST;

typedef struct _CD_MODULE_STRUCT {
    DWORD32 ModuleNameLen;
    UCHAR ModuleName[256];
    DWORD32 ModuleID;
    DWORD64 BaseAddr;
} CD_MODULE_STRUCT, * PCD_MODULE_STRUCT;

typedef struct _CD_MODULE_LIST {
    LIST_ENTRY          ListEntry;
    CD_MODULE_STRUCT     module;
}CD_MODULE_LIST, * PCD_MODULE_LIST;

typedef struct _CD_MODULE_OUTPUT_STRUCT {
    DWORD32 ModuleListSizeInBytes;
    DWORD32 NumberOfModules;
    UCHAR Modules[1]; // array length is determined by NumberOfModules * ModuleListSize
} CD_MODULE_OUTPUT_STRUCT, * PCD_MODULE_OUTPUT_STRUCT;

typedef struct _CD_HOOKS_OUTPUT_STRUCT {
    DWORD32 HooksListSizeInBytes;
    DWORD32 NumberOfHooks;
    UCHAR Hooks[1]; // array length is determined by NumberOfHooks * HooksListSizeInBytes
} CD_HOOKS_OUTPUT_STRUCT, * PCD_HOOKS_OUTPUT_STRUCT;

typedef struct _CD_READ_INPUT {
    DWORD64 ReadAddr;
    DWORD64 BufferSize;
} CD_READ_INPUT, * PCD_READ_INPUT;

typedef struct _CD_READ_OUTPUT {
    DWORD64 ReadAddr;
    DWORD32 BufferSize;
    DWORD32 NumChunks;
    DWORD32 DataLen;
    UCHAR Data[256];
};

typedef struct _CD_WRITE_INPUT {
    DWORD64 WriteAddr;
    DWORD32 BufferSize;
    DWORD32 NumChunks;
    DWORD32 DataLen;
    UCHAR Data[256];
};


#pragma pack ( pop )

#endif //_CHEATDRIVER_COMMON_H_
