// Background Windows listener for the Tang Nano 20K NEC remote decoder.
// Built with the Windows GUI subsystem: it has no console and lives in the
// notification area until the user selects Exit from its tray menu.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <array>
#include <atomic>
#include <string>
#include <thread>

#include "resource.h"

#pragma comment(lib, "shell32.lib")

namespace {

constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kConnectionMessage = WM_APP + 2;
constexpr UINT kExitCommand = 1;

std::atomic_bool g_running{true};

enum class Action { None, VolumeUp, VolumeDown, MediaPlayPause,
    Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9 };

struct RemoteButton {
    const char* frame;
    const char* name;
    Action action;
};

// All codes supplied in remote_button_mapping are recognized. Only the two
// volume buttons have an assigned Windows action at present.
constexpr std::array<RemoteButton, 46> kButtons{{
    {"NEC BE41FE01", "USB/SD", Action::None},
    {"NEC BD42FE01", "Power", Action::None},
    {"NEC BF40FE01", "2.1/5.1", Action::None},
    {"NEC B946FE01", "PC", Action::None},
    {"NEC BA45FE01", "FM", Action::None},
    {"NEC BB44FE01", "AUX", Action::None},
    {"NEC FB04FE01", "DVD", Action::None},
    {"NEC B54AFE01", "SET", Action::None},
    {"NEC B748FE01", "CLOCK", Action::None},
    {"NEC B649FE01", "TUNE+ [Up]", Action::None},
    {"NEC B14EFE01", "Previous CH- [Left]", Action::None},
    {"NEC B24DFE01", "Start/Pause Auto [Center]", Action::MediaPlayPause},
    {"NEC B34CFE01", "Forward CH+ [Right]", Action::None},
    {"NEC F20DFE01", "TUNE- [Down]", Action::None},
    {"NEC F00FFE01", "SLEEP", Action::None},
    {"NEC F30CFE01", "SAVE", Action::None},
    {"NEC FC03FE01", "FL-", Action::None},
    {"NEC FD02FE01", "FL+", Action::None},
    {"NEC FE01FE01", "FR-", Action::None},
    {"NEC FF00FE01", "FR+", Action::None},
    {"NEC F40BFE01", "SW+", Action::None},
    {"NEC F50AFE01", "CEN+", Action::None},
    {"NEC F609FE01", "VOL+", Action::VolumeUp},
    {"NEC F807FE01", "SW-", Action::None},
    {"NEC F906FE01", "CEN-", Action::None},
    {"NEC FA05FE01", "VOL-", Action::VolumeDown},
    {"NEC EC13FE01", "SL-", Action::None},
    {"NEC ED12FE01", "SL+", Action::None},
    {"NEC EE11FE01", "SR-", Action::None},
    {"NEC EF10FE01", "SR+", Action::None},
    {"NEC AD52FE01", "1", Action::Key1},
    {"NEC AE51FE01", "2", Action::Key2},
    {"NEC AF50FE01", "3", Action::Key3},
    {"NEC E817FE01", "4", Action::Key4},
    {"NEC E916FE01", "5", Action::Key5},
    {"NEC EA15FE01", "6", Action::Key6},
    {"NEC A956FE01", "7", Action::Key7},
    {"NEC AA55FE01", "8", Action::Key8},
    {"NEC AB54FE01", "9", Action::Key9},
    {"NEC A55AFE01", "REPEAT", Action::None},
    {"NEC A659FE01", "0", Action::Key0},
    {"NEC A758FE01", "RESET", Action::None},
    {"NEC E41BFE01", "TREBLE-", Action::None},
    {"NEC E51AFE01", "TREBLE+", Action::None},
    {"NEC E619FE01", "BASS-", Action::None},
    {"NEC E718FE01", "BASS+", Action::None},
}};

void update_tray_tip(HWND window, bool connected) {
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof(icon);
    icon.hWnd = window;
    icon.uID = kTrayIconId;
    icon.uFlags = NIF_TIP;
    lstrcpynW(icon.szTip,
              connected ? L"IR Remote Volume - connected to COM6"
                        : L"IR Remote Volume - waiting for COM6",
              ARRAYSIZE(icon.szTip));
    Shell_NotifyIconW(NIM_MODIFY, &icon);
}

void add_tray_icon(HWND window) {
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof(icon);
    icon.hWnd = window;
    icon.uID = kTrayIconId;
    icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    icon.uCallbackMessage = kTrayCallbackMessage;

    // Get the instance handle of the current executable
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Load the icon from the embedded resources
    icon.hIcon = (HICON)LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_APP_ICON), // ID from your resource.h
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), // Get standard small icon width
        GetSystemMetrics(SM_CYSMICON), // Get standard small icon height
        LR_DEFAULTCOLOR
    );

    lstrcpynW(icon.szTip, L"IR Remote Volume - waiting for COM6",
              ARRAYSIZE(icon.szTip));
    Shell_NotifyIconW(NIM_ADD, &icon);
}

void remove_tray_icon(HWND window) {
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof(icon);
    icon.hWnd = window;
    icon.uID = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &icon);
}

void show_tray_menu(HWND window) {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) return;
    AppendMenuW(menu, MF_STRING, kExitCommand, L"Exit");
    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, window, nullptr);
    DestroyMenu(menu);
}

void send_volume_key(WORD key) {
    INPUT inputs[2]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = key;
    inputs[1] = inputs[0];
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(static_cast<UINT>(std::size(inputs)), inputs, sizeof(INPUT));
}

void handle_line(const std::string& line) {
    for (const auto& button : kButtons) {
        if (line != button.frame) continue;
        if (button.action == Action::VolumeUp) send_volume_key(VK_VOLUME_UP);
        if (button.action == Action::VolumeDown) send_volume_key(VK_VOLUME_DOWN);
        if (button.action == Action::MediaPlayPause) send_volume_key(VK_MEDIA_PLAY_PAUSE);
        if(button.action == Action::Key1) send_volume_key(VK_NUMPAD1);
        if(button.action == Action::Key2) send_volume_key(VK_NUMPAD2);
        if(button.action == Action::Key3) send_volume_key(VK_NUMPAD3);
        if(button.action == Action::Key4) send_volume_key(VK_NUMPAD4);
        if(button.action == Action::Key5) send_volume_key(VK_NUMPAD5);
        if(button.action == Action::Key6) send_volume_key(VK_NUMPAD6);
        if(button.action == Action::Key7) send_volume_key(VK_NUMPAD7);
        if(button.action == Action::Key8) send_volume_key(VK_NUMPAD8);
        if(button.action == Action::Key9) send_volume_key(VK_NUMPAD9);
        if(button.action == Action::Key0) send_volume_key(VK_NUMPAD0);
        return;
    }
    // IR READY and unknown/malformed input are deliberately ignored.
}

HANDLE open_serial_port() {
    HANDLE port = CreateFileW(L"\\\\.\\COM6", GENERIC_READ, 0, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (port == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    DCB settings{};
    settings.DCBlength = sizeof(settings);
    if (!GetCommState(port, &settings)) {
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }
    settings.BaudRate = CBR_115200;
    settings.ByteSize = 8;
    settings.Parity = NOPARITY;
    settings.StopBits = ONESTOPBIT;
    settings.fBinary = TRUE;
    settings.fParity = FALSE;
    settings.fOutxCtsFlow = FALSE;
    settings.fOutxDsrFlow = FALSE;
    settings.fDtrControl = DTR_CONTROL_DISABLE;
    settings.fRtsControl = RTS_CONTROL_DISABLE;
    settings.fOutX = FALSE;
    settings.fInX = FALSE;

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 500;
    if (!SetCommState(port, &settings) || !SetCommTimeouts(port, &timeouts)) {
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }
    PurgeComm(port, PURGE_RXABORT | PURGE_RXCLEAR);
    return port;
}

void read_serial_port(HANDLE port) {
    std::string line;
    char buffer[64];
    while (g_running) {
        DWORD bytes_read = 0;
        if (!ReadFile(port, buffer, sizeof(buffer), &bytes_read, nullptr)) return;
        for (DWORD i = 0; i < bytes_read; ++i) {
            const char character = buffer[i];
            if (character == '\r') continue;
            if (character == '\n') {
                handle_line(line);
                line.clear();
            } else if (line.size() < 64) {
                line += character;
            } else {
                line.clear(); // Drop malformed overlong input until newline.
            }
        }
    }
}

void serial_worker(HWND window) {
    while (g_running) {
        HANDLE port = open_serial_port();
        if (port == INVALID_HANDLE_VALUE) {
            PostMessageW(window, kConnectionMessage, FALSE, 0);
            Sleep(2000);
            continue;
        }
        PostMessageW(window, kConnectionMessage, TRUE, 0);
        read_serial_port(port);
        CloseHandle(port);
        PostMessageW(window, kConnectionMessage, FALSE, 0);
        Sleep(2000);
    }
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case kTrayCallbackMessage:
        if (lparam == WM_RBUTTONUP || lparam == WM_CONTEXTMENU) show_tray_menu(window);
        return 0;
    case kConnectionMessage:
        update_tray_tip(window, wparam != FALSE);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wparam) == kExitCommand) DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        remove_tray_icon(window);
        g_running = false;
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    const wchar_t window_class_name[] = L"IrRemoteVolumeTrayWindow";
    WNDCLASSW window_class{};
    window_class.hInstance = instance;
    window_class.lpfnWndProc = window_proc;
    window_class.lpszClassName = window_class_name;
    RegisterClassW(&window_class);

    HWND window = CreateWindowExW(0, window_class_name, L"IR Remote Volume",
                                  WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr,
                                  nullptr, instance, nullptr);
    if (window == nullptr) return 1;

    add_tray_icon(window);
    std::thread serial_thread(serial_worker, window);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    g_running = false;
    serial_thread.join();
    return 0;
}
