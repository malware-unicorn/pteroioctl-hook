# pteroioctl-hook
A driver to implement IOCTL hooking

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

