# pteroioctl-hook
A driver to implement IOCTL hooking

Warning: In progress

```
                           <\              _
                            \\          _/{
                     _       \\       _-   -_
                   /{        / `\   _-     - -_
                 _~  =      ( @  \ -        -  -_
               _- -   ~-_   \( =\ \           -  -_
             _~  -       ~_ | 1 :\ \      _-~-_ -  -_
           _-   -          ~  |V: \ \  _-~     ~-_-  -_
        _-~   -            /  | :  \ \            ~-_- -_
     _-~    -   _.._      {   | : _-``               ~- _-_
  _-~   -__..--~    ~-_  {   : \:}
=~__.--~~              ~-_\  :  /
                           \ : /__
                          //`Y'--\\      = IOCTL
                         <+       \\
                          \\      WWW
                          MMM
```

### Loading
```
CheatController.exe -load
```

### Options
```
>>>> Cheat Console:

        1                List Modules
        2                Select Module
        3                List Hooks
        4                Place Hook (Inserts Into list, TODO: set hook)
        5                Kernel Memory Read (TODO)
        6                Kernel Memory Write (TODO)

        0                Exit
```

## Features
1) Search for existing modules
2) Select module by ID
3) List Hooks
4) Add a new hook to the selected module (TODO: Implment control for hook)
5) TODO: read memory
6) TODO: write memory

## Testing
Tested on Windows 10 build 17763.rs5 with TestSigning On
```
bcdedit /set testsigning on
```
Adding cert command
```
certmgr.exe /add CheatDriver.cer /s /r localMachine root
```

## Customizing Hooks
Uses a generic hook structure to instrument a jmp
```
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
```

Example Usage:
```
CD_HOOK_STRUCT newHook = { };
UCHAR JUMP_RAX[] = "\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xE0";

// This is a test, address are dummy addresses
newHook.Hook.EntryAddr = 0x1234; // This should be physical address not an offset
newHook.Hook.JumpAddr = 0x4567; // This should be physical address not an offset
newHook.Hook.ReturnAddr = 0x9123; // This should be physical address not an offset
newHook.Hook.IsCustomHook = TRUE;
memcpy(newHook.Hook.CustomHookBuffer, JUMP_RAX, sizeof(JUMP_RAX));
newHook.Hook.CustomHookLen = sizeof(JUMP_RAX);
newHook.Hook.CustomHookAddrOffset = 2;
```

