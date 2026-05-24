@echo off
setlocal

set OUT=cursor-begone.exe
set SRC=cursor_hider.cpp
set RC=cursor_hider.rc
set LIBS=user32.lib shell32.lib advapi32.lib

:: ── Try MSVC (cl.exe + rc.exe) ─────────────────────────────────────────────
where cl >nul 2>&1
if %errorlevel% == 0 (
    echo [build] using MSVC
    rc /nologo /fo cursor_hider.res %RC% || goto :err
    cl /nologo /W3 /O1 /GS- /GR- /EHs- %SRC% cursor_hider.res ^
       /link %LIBS% /subsystem:windows /OPT:REF /OPT:ICF /out:%OUT%
    if %errorlevel% == 0 echo [build] done: %OUT%
    del cursor_hider.res 2>nul
    goto :end
)

:: ── Try MinGW g++ + windres ────────────────────────────────────────────────
where g++ >nul 2>&1
if %errorlevel% == 0 (
    echo [build] using MinGW g++
    windres %RC% -O coff -o cursor_hider.res.o || goto :err
    g++ -Os -s -mwindows -fno-exceptions -fno-rtti ^
        -ffunction-sections -fdata-sections ^
        -Wl,--gc-sections -Wl,--strip-all ^
        -o %OUT% %SRC% cursor_hider.res.o -luser32 -lshell32 -ladvapi32
    if %errorlevel% == 0 echo [build] done: %OUT%
    del cursor_hider.res.o 2>nul
    goto :end
)

:: ── Try LLVM clang-cl + rc.exe ─────────────────────────────────────────────
where clang-cl >nul 2>&1
if %errorlevel% == 0 (
    echo [build] using clang-cl
    rc /nologo /fo cursor_hider.res %RC% || goto :err
    clang-cl /W3 /O2 %SRC% cursor_hider.res /link %LIBS% /subsystem:windows /out:%OUT%
    if %errorlevel% == 0 echo [build] done: %OUT%
    del cursor_hider.res 2>nul
    goto :end
)

echo [build] ERROR: no compiler found (cl, g++, or clang-cl)
exit /b 1

:err
echo [build] resource compile failed
exit /b 1

:end
endlocal
