#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#define OEMRESOURCE       // OCR_NORMAL
#define NOMINMAX          // no min/max macros (they break std::min/max)
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOMETAFILE        // typedef METAFILEPICT
#define NOOPENFILE        // OpenFile, OemToAnsi, etc.
#define NOSERVICE         // service controller
#define NOSOUND           // sound driver
#define NOCOMM            // COMM driver
#define NOKANJI           // Kanji support
#define NOHELP            // help engine interface
#define NOPROFILER        // profiler interface
#define NODEFERWINDOWPOS  // SetWindowPos and friends
#define NOMCX             // Modem Configuration Extensions
#define NOIME             // input method editor
#define NOCRYPT           // wincrypt.h
#define NORASTEROPS       // raster operation defines
#define NOATOM            // Atom Manager
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // screen colors
#define NODRAWTEXT        // DrawText and DT_*
#define NOKERNEL          // (NB: we still get CreateMutex etc. — these are filtered macros only)
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, etc.
#define NOSCROLL          // SB_* and scrolling routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
// NOTE: we do NOT define NOWH — we need WH_MOUSE_LL / SetWindowsHookEx
#include <windows.h>
#include <shellapi.h>

#define WM_TRAY        (WM_USER + 1)
#define CMD_EXIT       1001
#define CMD_AUTOSTART  1002
#define HIDE_TIMER     1
#define HIDE_MS        3000

static const wchar_t* RUN_KEY   = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* RUN_VALUE = L"cursor-begone";

static HHOOK          s_hook;
static HWND           s_hwnd;
static HCURSOR        s_blank;
static bool           s_hidden;
static NOTIFYICONDATA s_nid;

static bool IsAutostartEnabled() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS)
        return false;
    LSTATUS r = RegQueryValueExW(k, RUN_VALUE, nullptr, nullptr, nullptr, nullptr);
    RegCloseKey(k);
    return r == ERROR_SUCCESS;
}

static void SetAutostart(bool on) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &k) != ERROR_SUCCESS)
        return;
    if (on) {
        wchar_t path[MAX_PATH + 2];
        path[0] = L'"';
        DWORD len = GetModuleFileNameW(nullptr, path + 1, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            path[len + 1] = L'"';
            path[len + 2] = 0;
            RegSetValueExW(k, RUN_VALUE, 0, REG_SZ,
                (const BYTE*)path, (DWORD)((len + 3) * sizeof(wchar_t)));
        }
    } else {
        RegDeleteValueW(k, RUN_VALUE);
    }
    RegCloseKey(k);
}

static HCURSOR MakeBlankCursor() {
    int w = GetSystemMetrics(SM_CXCURSOR);
    int h = GetSystemMetrics(SM_CYCURSOR);
    int n = ((w + 7) / 8) * h;
    BYTE andMask[8192], xorMask[8192];
    if (n <= 0 || n > (int)sizeof(andMask)) return nullptr;
    for (int i = 0; i < n; ++i) { andMask[i] = 0xFF; xorMask[i] = 0; }
    return CreateCursor(GetModuleHandleW(nullptr), 0, 0, w, h, andMask, xorMask);
}

static void HideCursor() {
    if (s_hidden) return;
    HCURSOR copy = CopyCursor(s_blank);
    if (!copy) return;
    // SetSystemCursor consumes the handle on success only; clean up on failure
    if (SetSystemCursor(copy, OCR_NORMAL))
        s_hidden = true;
    else
        DestroyCursor(copy);
}

static void RestoreCursor() {
    if (s_hidden) {
        SystemParametersInfoW(SPI_SETCURSORS, 0, nullptr, 0);
        s_hidden = false;
    }
}

static LRESULT CALLBACK MouseHook(int code, WPARAM w, LPARAM l) {
    if (code == HC_ACTION && w == WM_MOUSEMOVE) {
        RestoreCursor();
        SetTimer(s_hwnd, HIDE_TIMER, HIDE_MS, nullptr);
    }
    return CallNextHookEx(nullptr, code, w, l);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_TIMER:
        KillTimer(hwnd, HIDE_TIMER);
        HideCursor();
        break;
    case WM_TRAY:
        if (l == WM_RBUTTONUP) {
            POINT pt; GetCursorPos(&pt);
            HMENU m = CreatePopupMenu();
            UINT autoFlags = MF_STRING | (IsAutostartEnabled() ? MF_CHECKED : MF_UNCHECKED);
            AppendMenuW(m, autoFlags,     CMD_AUTOSTART, L"Start with Windows");
            AppendMenuW(m, MF_SEPARATOR,  0,             nullptr);
            AppendMenuW(m, MF_STRING,     CMD_EXIT,      L"Exit cursor-begone");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(m);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case CMD_EXIT:      PostQuitMessage(0);                       break;
        case CMD_AUTOSTART: SetAutostart(!IsAutostartEnabled());      break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, w, l);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    CreateMutexW(nullptr, FALSE, L"cursor-begone-mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    s_blank = MakeBlankCursor();
    if (!s_blank) return 1;

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hi;
    wc.lpszClassName = L"_CursorBegone";
    RegisterClassW(&wc);

    s_hwnd = CreateWindowW(L"_CursorBegone", nullptr, 0,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, hi, nullptr);
    if (!s_hwnd) return 1;

    s_nid.cbSize           = sizeof(s_nid);
    s_nid.hWnd             = s_hwnd;
    s_nid.uID              = 1;
    s_nid.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    s_nid.uCallbackMessage = WM_TRAY;
    s_nid.hIcon            = LoadIconW(hi, MAKEINTRESOURCEW(1));
    if (!s_nid.hIcon) s_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(s_nid.szTip, L"cursor-begone -- right-click to exit", 128);
    Shell_NotifyIconW(NIM_ADD, &s_nid);

    s_hook = SetWindowsHookExW(WH_MOUSE_LL, MouseHook, nullptr, 0);
    if (!s_hook) {
        MessageBoxW(nullptr,
            L"Failed to install the global mouse hook.\n"
            L"cursor-begone cannot run.",
            L"cursor-begone", MB_OK | MB_ICONERROR);
        Shell_NotifyIconW(NIM_DELETE, &s_nid);
        DestroyCursor(s_blank);
        return 1;
    }
    SetTimer(s_hwnd, HIDE_TIMER, HIDE_MS, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RestoreCursor();
    Shell_NotifyIconW(NIM_DELETE, &s_nid);
    UnhookWindowsHookEx(s_hook);
    DestroyCursor(s_blank);
    return 0;
}
