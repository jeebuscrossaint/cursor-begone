# cursor-begone

Tiny Windows tray app that hides the mouse cursor after **3 seconds** of inactivity and restores it the moment you move the mouse. Stripped release binary is **~24 KB**.

## Install

`cursor-begone.exe` is committed to the repo — download it directly or build from source (below). Drop it anywhere, double-click to run. It lives in the system tray.

## Tray menu

Right-click the tray icon:

- **Start with Windows** — toggles a `HKCU\…\Run` entry pointing to the current exe path
- **Exit** — restores the cursor and quits cleanly

Running it a second time is a no-op (single-instance guard via named mutex).

## Build

Any of:

```sh
build.bat                                   # auto-detects MSVC / MinGW / clang-cl
cmake -B build && cmake --build build       # cross-toolchain via CMake
g++ -Os -s -mwindows cursor_hider.cpp cursor_hider.rc -o cursor-begone.exe \
    -luser32 -lshell32 -ladvapi32           # direct MinGW one-liner (rc via windres)
```

Requires only a C++ compiler, the Windows SDK / MinGW headers, and `windres`/`rc.exe` for the icon resource.

## Regenerating the icon

```sh
python tools/make_icon.py     # needs Pillow → writes assets/cursor-begone.ico
```

## How it works

A low-level mouse hook (`WH_MOUSE_LL`) catches movement system-wide and resets a 3 s timer; on expiry, `SetSystemCursor` swaps the arrow for an empty cursor. On the next move, `SystemParametersInfo(SPI_SETCURSORS)` reloads cursors from the active theme.

## Caveat

Applications that explicitly call `SetCursor()` for their own window (some games, certain browser hover states) can override the system cursor temporarily. Outside those, the cursor stays gone.

## License

MIT
