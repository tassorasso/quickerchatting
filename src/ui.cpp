#include "ui.h"
#include "utils.h"

void FlashTaskbar() {
    if (GetForegroundWindow() != MainAppWindow) {
        FLASHWINFO fi;
        fi.cbSize = sizeof(FLASHWINFO);
        fi.hwnd = MainAppWindow;
        fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
        fi.uCount = 3;
        fi.dwTimeout = 0;
        FlashWindowEx(&fi);
    }
}

void SelectColor(HWND h, COLORREF& target) {
    CHOOSECOLOR cc;
    static COLORREF cust[16];
    memset(&cc, 0, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = h;
    cc.lpCustColors = cust;
    cc.rgbResult = target;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc)) target = cc.rgbResult;
}

void ToggleUI(bool v) {
    int mode = v ? SW_SHOW : SW_HIDE;
    ShowWindow(TypeBox, mode);
    ShowWindow(SendBtn, mode);
    ShowWindow(PinBox, mode);
    ShowWindow(UsersList, mode);
    ShowWindow(SmallLabel, mode);
    ShowWindow(MemberLabel, mode);
    ShowWindow(OptButton, mode);
}

void SetGhost(bool mode) {
    if (!AmIConnected) return;
    Invisible = mode;
    if (mode) {
        SetWindowLong(MainAppWindow, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);
        ToggleUI(true);
        SetLayeredWindowAttributes(MainAppWindow, 0, 230, LWA_ALPHA);
        DeleteObject(BackgroundBrush);
        BackgroundBrush = CreateSolidBrush(color_bg);
        SetClassLongPtr(MainAppWindow, GCLP_HBRBACKGROUND, (LONG_PTR)BackgroundBrush);
        if(ChatArea) {
            SetWindowLong(ChatArea, GWL_EXSTYLE, WS_EX_STATICEDGE);
            SendMessage(ChatArea, EM_SETBKGNDCOLOR, 0, color_bg);
            SendMessage(ChatArea, EM_SHOWSCROLLBAR, SB_VERT, TRUE);
        }
    } else {
        SetWindowLong(MainAppWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        ToggleUI(false);
        DeleteObject(BackgroundBrush);
        BackgroundBrush = CreateSolidBrush(color_trans);
        SetClassLongPtr(MainAppWindow, GCLP_HBRBACKGROUND, (LONG_PTR)BackgroundBrush);
        if(ChatArea) {
            SetWindowLong(ChatArea, GWL_EXSTYLE, 0);
            SendMessage(ChatArea, EM_SETBKGNDCOLOR, 0, color_trans);
            SendMessage(ChatArea, EM_SHOWSCROLLBAR, SB_VERT, FALSE);
        }
        SetLayeredWindowAttributes(MainAppWindow, color_trans, 0, LWA_COLORKEY);
    }
    SetWindowPos(MainAppWindow, 0,0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
    InvalidateRect(MainAppWindow, NULL, TRUE);
}

void PushText(string t, COLORREF c, bool b) {
    if (!ChatArea) return;
    int length = GetWindowTextLength(ChatArea);
    SendMessage(ChatArea, EM_SETSEL, length, length);
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = c;
    cf.dwEffects = b ? CFE_BOLD : 0;
    SendMessage(ChatArea, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageA(ChatArea, EM_REPLACESEL, FALSE, (LPARAM)(t.c_str()));
    SendMessage(ChatArea, WM_VSCROLL, SB_BOTTOM, 0);
    if (!Invisible) SendMessage(ChatArea, EM_SHOWSCROLLBAR, SB_VERT, FALSE);
}

void ShowSys(string m) {
    PushText(TimeString() + m + "\r\n", color_sys, false);
}

void ShowChat(string user, string m, bool me, bool w) {
    PushText(TimeString(), color_sys, false);
    COLORREF c = color_them;
    if (me) c = color_me;
    if (w) c = color_private;
    string final_n = user;
    if (w) final_n = me ? "(To " + user + ")" : "(From " + user + ")";
    PushText(final_n + ": ", c, true);
    PushText(m + "\r\n", w ? color_private : color_text, false);
    if (!me) FlashTaskbar();
}

void RefreshUsers(string raw) {
    SendMessage(UsersList, LB_RESETCONTENT, 0, 0);
    stringstream ss(raw);
    string n;
    while (getline(ss, n, ',')) {
        SendMessage(UsersList, LB_ADDSTRING, 0, (LPARAM)n.c_str());
    }
}

void BuildChatUI(HWND h) {
    HWND child = GetWindow(h, GW_CHILD);
    while (child) { ShowWindow(child, SW_HIDE); child = GetWindow(child, GW_HWNDNEXT); }
    AmIConnected = true;
    SetWindowPos(h, NULL, 0, 0, 750, 500, SWP_NOMOVE | SWP_NOZORDER);
    SetGhost(true);
    ChatArea = CreateWindowEx(WS_EX_STATICEDGE, "RichEdit20A", "", WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY, 15, 15, 520, 380, h, (HMENU)101, GetModuleHandle(NULL), NULL);
    SendMessage(ChatArea, EM_SETBKGNDCOLOR, 0, color_bg);
    MemberLabel = CreateWindow("STATIC", "Members:", WS_CHILD|WS_VISIBLE, 550, 15, 100, 20, h, (HMENU)114, NULL, NULL);
    UsersList = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD|WS_VISIBLE|LBS_STANDARD, 550, 35, 160, 360, h, (HMENU)110, GetModuleHandle(NULL), NULL);
    TypeBox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, 15, 410, 420, 30, h, (HMENU)102, NULL, NULL);
    SendBtn = CreateWindow("BUTTON", "SEND", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|BS_FLAT, 445, 409, 90, 32, h, (HMENU)103, NULL, NULL);
    PinBox = CreateWindow("BUTTON", "Pin", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_FLAT, 550, 415, 50, 20, h, (HMENU)109, NULL, NULL);
    OptButton = CreateWindow("BUTTON", "Settings", WS_CHILD|WS_VISIBLE|BS_FLAT, 610, 410, 100, 30, h, (HMENU)111, NULL, NULL);
    SmallLabel = CreateWindow("STATIC", "Public Chat", WS_CHILD|WS_VISIBLE, 15, 395, 300, 15, h, (HMENU)113, NULL, NULL);
    SendMessage(ChatArea, WM_SETFONT, (WPARAM)f1, TRUE);
    SendMessage(MemberLabel, WM_SETFONT, (WPARAM)f1, TRUE);
    SendMessage(UsersList, WM_SETFONT, (WPARAM)f1, TRUE);
    SendMessage(TypeBox, WM_SETFONT, (WPARAM)f1, TRUE);
    SendMessage(SendBtn, WM_SETFONT, (WPARAM)f2, TRUE);
    SendMessage(PinBox, WM_SETFONT, (WPARAM)f1, TRUE);
    SendMessage(OptButton, WM_SETFONT, (WPARAM)f1, TRUE);
    SendMessage(SmallLabel, WM_SETFONT, (WPARAM)f1, TRUE);
    SetFocus(TypeBox);
}