#include "globals.h"
#include "utils.h"
#include "ui.h"
#include "network.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

LRESULT CALLBACK MainProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        f1 = CreateFont(19,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Segoe UI");
        f2 = CreateFont(19,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Segoe UI");
        f3 = CreateFont(26,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Segoe UI");
        HWND t = CreateWindow("STATIC", "Party Chat", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 15, 560, 35, h, NULL, NULL, NULL);
        SendMessage(t, WM_SETFONT, (WPARAM)f3, TRUE);
        HWND grp = CreateWindow("BUTTON", "Connection Setup", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 60, 545, 230, h, (HMENU)112, NULL, NULL);
        SendMessage(grp, WM_SETFONT, (WPARAM)f1, TRUE);
        CreateWindow("STATIC", "Display Name:", WS_CHILD | WS_VISIBLE, 80, 95, 100, 20, h, NULL, NULL, NULL);
        HWND name = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 190, 92, 350, 26, h, (HMENU)107, NULL, NULL);
        SendMessage(name, WM_SETFONT, (WPARAM)f1, TRUE);
        CreateWindow("STATIC", "Password:", WS_CHILD | WS_VISIBLE, 80, 135, 100, 20, h, NULL, NULL, NULL);
        PassBox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD, 190, 132, 350, 26, h, (HMENU)106, NULL, NULL);
        SendMessage(PassBox, WM_SETFONT, (WPARAM)f1, TRUE);
        CreateWindow("STATIC", "Host IP:", WS_CHILD | WS_VISIBLE, 80, 175, 100, 20, h, NULL, NULL, NULL);
        HWND ip = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 190, 172, 350, 26, h, (HMENU)108, NULL, NULL);
        SendMessage(ip, WM_SETFONT, (WPARAM)f1, TRUE);
        CreateWindow("STATIC", "Port:", WS_CHILD | WS_VISIBLE, 80, 215, 100, 20, h, NULL, NULL, NULL);
        PortBox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "25565", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER, 190, 212, 100, 26, h, (HMENU)116, NULL, NULL);
        SendMessage(PortBox, WM_SETFONT, (WPARAM)f1, TRUE);
        PublicCheck = CreateWindow("BUTTON", "Public Server (No Password)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_FLAT, 190, 250, 200, 20, h, (HMENU)115, NULL, NULL);
        SendMessage(PublicCheck, WM_SETFONT, (WPARAM)f1, TRUE);
        HWND host = CreateWindow("BUTTON", "Host Party", WS_CHILD | WS_VISIBLE | BS_FLAT, 40, 300, 240, 40, h, (HMENU)104, NULL, NULL);
        HWND join = CreateWindow("BUTTON", "Join Party", WS_CHILD | WS_VISIBLE | BS_FLAT, 300, 300, 240, 40, h, (HMENU)105, NULL, NULL);
        SendMessage(host, WM_SETFONT, (WPARAM)f2, TRUE);
        SendMessage(join, WM_SETFONT, (WPARAM)f2, TRUE);
        SetDlgItemText(h, 108, "127.0.0.1");
        break;
    }
    case WM_HOTKEY:
        if (w == 9000) {
            if (IsIconic(h)) ShowWindow(h, SW_RESTORE);
            SetForegroundWindow(h);
            SetFocus(TypeBox);
            SetGhost(true);
        }
        break;
    case WM_ACTIVATE:
        if (AmIConnected) {
            if (LOWORD(w) == WA_INACTIVE) SetGhost(false);
            else SetGhost(true);
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        SetTextColor((HDC)w, RGB(50,50,60));
        SetBkMode((HDC)w, TRANSPARENT);
        return (INT_PTR)BackgroundBrush;
    case WM_COMMAND:
        if (LOWORD(w) == 115) {
            bool c = (SendMessage(PublicCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(PassBox, !c);
            IsPublic = c;
        }
        else if (LOWORD(w) == 104) {
            char p[256], n[256];
            GetDlgItemText(h, 106, p, 256);
            GetDlgItemText(h, 107, n, 256);
            int port = GetDlgItemInt(h, 116, NULL, FALSE);
            if (port == 0) port = 25565;
            if(strlen(n) > 0) { BuildChatUI(h); StartHost(p, n, port); }
        }
        else if (LOWORD(w) == 105) {
            char i[256], p[256], n[256];
            GetDlgItemText(h, 108, i, 256);
            GetDlgItemText(h, 106, p, 256);
            GetDlgItemText(h, 107, n, 256);
            int port = GetDlgItemInt(h, 116, NULL, FALSE);
            if (port == 0) port = 25565;
            if(strlen(i) > 0 && strlen(n) > 0) { BuildChatUI(h); StartJoin(i, p, n, port); }
        }
        else if (LOWORD(w) == 103) {
            char buf[1024]; GetDlgItemText(h, 102, buf, 1024);
            HandleSend(h, buf);
            SetFocus(TypeBox);
        }
        else if (LOWORD(w) == 109) {
            bool c = (SendMessage(PinBox, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SetWindowPos(MainAppWindow, c ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
        }
        else if (LOWORD(w) == 111) {
            HMENU m = CreatePopupMenu();
            HMENU sub = CreatePopupMenu();
            AppendMenu(sub, MF_STRING, 301, "Me");
            AppendMenu(sub, MF_STRING, 302, "Friends");
            AppendMenu(sub, MF_STRING, 303, "System");
            AppendMenu(sub, MF_STRING, 304, "Whisper");
            AppendMenu(sub, MF_STRING, 305, "Text");
            AppendMenu(m, MF_POPUP, (UINT_PTR)sub, "Colors");
            HMENU hk = CreatePopupMenu();
            AppendMenu(hk, MF_STRING, 400, "None");
            AppendMenu(hk, MF_STRING, 401, "F1");
            AppendMenu(hk, MF_STRING, 402, "Insert");
            AppendMenu(hk, MF_STRING, 403, "Alt + C");
            AppendMenu(m, MF_POPUP, (UINT_PTR)hk, "Hotkey");
            AppendMenu(m, MF_SEPARATOR, 0, NULL);
            if(HostingNow) {
                AppendMenu(m, MF_STRING | (BadWordFilter ? MF_CHECKED : 0), 201, "Bad words");
                AppendMenu(m, MF_STRING | (NoSpam ? MF_CHECKED : 0), 203, "Anti Spam");
            }
            AppendMenu(m, MF_STRING, 202, "Save Log");
            RECT rc; GetWindowRect(GetDlgItem(h, 111), &rc);
            TrackPopupMenu(m, TPM_RIGHTBUTTON, rc.left, rc.bottom, 0, h, NULL);
            DestroyMenu(m);
        }
        else if (LOWORD(w) == 301) SelectColor(h, color_me);
        else if (LOWORD(w) == 302) SelectColor(h, color_them);
        else if (LOWORD(w) == 303) SelectColor(h, color_sys);
        else if (LOWORD(w) == 304) SelectColor(h, color_private);
        else if (LOWORD(w) == 305) SelectColor(h, color_text);
        else if (LOWORD(w) >= 400 && LOWORD(w) <= 403) {
            UnregisterHotKey(MainAppWindow, 9000);
            int vk = 0, mod = 0;
            if (LOWORD(w) == 401) vk = VK_F1;
            else if (LOWORD(w) == 402) vk = VK_INSERT;
            else if (LOWORD(w) == 403) { vk = 'C'; mod = MOD_ALT; }
            if(vk) RegisterHotKey(MainAppWindow, 9000, mod, vk);
        }
        else if (LOWORD(w) == 201 && HostingNow) {
            BadWordFilter = !BadWordFilter;
            ShowSys(BadWordFilter ? "Filter on." : "Filter off.");
        }
        else if (LOWORD(w) == 203 && HostingNow) {
            NoSpam = !NoSpam;
            ShowSys(NoSpam ? "Anti-spam on." : "Anti-spam off.");
        }
        else if (LOWORD(w) == 202) {
            MessageBox(h, "Saved to chat_log.txt", "OK", MB_OK);
        }
        break;
    case WM_CLOSE:
        AppRunning = false;
        DestroyWindow(h);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(h, m, w, l);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR a, int s) {
    WSADATA wd; WSAStartup(MAKEWORD(2,2), &wd);
    LoadLibrary("Riched20.dll");
    BackgroundBrush = CreateSolidBrush(color_bg);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = MainProc;
    wc.hInstance = h;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = BackgroundBrush;
    wc.lpszClassName = "Chat";
    RegisterClass(&wc);
    MainAppWindow = CreateWindowEx(WS_EX_LAYERED, "Chat", "Party Chat", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 600, 450, NULL, NULL, h, NULL);
    SetLayeredWindowAttributes(MainAppWindow, 0, 255, LWA_ALPHA);
    ShowWindow(MainAppWindow, s);
    MSG m;
    while(GetMessage(&m,0,0,0)) {
        if(m.message == WM_KEYDOWN && m.wParam == VK_RETURN && GetFocus() == TypeBox) {
            SendMessage(MainAppWindow, WM_COMMAND, 103, 0);
        } else {
            TranslateMessage(&m);
            DispatchMessage(&m);
        }
    }
    WSACleanup();
    return 0;
}