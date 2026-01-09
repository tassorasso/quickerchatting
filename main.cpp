#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <richedit.h> 
#include <urlmon.h> 
#include <string>
#include <thread>
#include <atomic>
#include <algorithm>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <mutex>
#include <iostream>
#include <chrono>

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "urlmon.lib") 

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifndef EM_SHOWSCROLLBAR
#define EM_SHOWSCROLLBAR (WM_USER + 96)
#endif

HWND MainAppWindow;
HWND ChatArea; 
HWND TypeBox;
HWND SendBtn;
HWND PinBox;
HWND UsersList; 
HWND SmallLabel;
HWND MemberLabel;
HWND OptButton;
HWND PublicCheck; 
HWND PassBox;
HWND PortBox;

SOCKET GlobalSock; 
bool AppRunning = false;
string MyName;
bool HostingNow = false;
bool AmIConnected = false; 
bool IsPublic = false; 
bool Invisible = false;

COLORREF color_bg = RGB(245, 247, 250); 
COLORREF color_trans = RGB(20, 20, 20); 
COLORREF color_me = RGB(0, 102, 204);     
COLORREF color_them = RGB(204, 0, 0);       
COLORREF color_sys = RGB(100, 100, 100);   
COLORREF color_private = RGB(128, 0, 128); 
COLORREF color_text = RGB(0, 0, 0); 

bool IsWhispering = false;
string TargetName = "";

bool BadWordFilter = true; 
bool NoSpam = true; 
vector<string> ListOfBadWords;

struct UserData {
    SOCKET sock;
    string name;
    int id;
    string lastMsg;
    chrono::steady_clock::time_point lastTime;
};

vector<UserData*> ConnectionList;
mutex LockThing; 
int TotalUsers = 0;

HFONT f1, f2, f3;
HBRUSH BackgroundBrush;

void LoadWords() {
    ListOfBadWords.push_back("damn");
    ListOfBadWords.push_back("hell");
    char path[MAX_PATH];
    if (URLDownloadToCacheFile(NULL, "m", path, MAX_PATH, 0, NULL) == S_OK) {
        ifstream f(path);
        string l;
        while(getline(f, l)) {
            if (!l.empty() && l.back() == '\r') l.pop_back();
            if(l.length() > 0) ListOfBadWords.push_back(l);
        }
    } else {
        ifstream f("badwords.txt");
        string l;
        while(getline(f, l)) ListOfBadWords.push_back(l);
    }
}

string TimeString() {
    time_t t = time(0);
    struct tm* now = localtime(&t);
    char b[100];
    sprintf(b, "[%02d:%02d:%02d] ", now->tm_hour, now->tm_min, now->tm_sec);
    return string(b);
}

string FixText(string input) {
    if (!BadWordFilter) return input;
    string low = input;
    transform(low.begin(), low.end(), low.begin(), ::tolower);
    for (int i = 0; i < ListOfBadWords.size(); i++) {
        string w = ListOfBadWords[i];
        if (w.length() == 0) continue; 
        size_t p = low.find(w);
        while (p != string::npos) {
            for(int j=0; j<w.length(); j++) {
                input[p+j] = '*';
                low[p+j] = '*';
            }
            p = low.find(w, p+1);
        }
    }
    return input;
}

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

void ShowChat(string user, string m, bool me, bool w = false) {
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

void TellEveryone(string msg, SOCKET skip) {
    LockThing.lock();
    for (int i=0; i<ConnectionList.size(); i++) {
        if (ConnectionList[i]->sock != skip) {
            send(ConnectionList[i]->sock, msg.c_str(), msg.length(), 0);
        }
    }
    LockThing.unlock();
}

void SendUserList() {
    string d = "<<USERS>>" + MyName; 
    LockThing.lock();
    for (auto c : ConnectionList) d += "," + c->name;
    for (auto c : ConnectionList) send(c->sock, d.c_str(), d.length(), 0);
    LockThing.unlock();
    RefreshUsers(d.substr(9)); 
}

void ClientManager(UserData* u) {
    char b[4096];
    while (AppRunning) {
        int r = recv(u->sock, b, 4095, 0);
        if (r <= 0) break;
        b[r] = 0;
        string m = b;
        if (m.find("/w ") == 0) {
            int gap = m.find(' ', 3);
            if (gap > 0) {
                string who = m.substr(3, gap - 3);
                string text = FixText(m.substr(gap + 1));
                bool ok = false;
                if (who == MyName) {
                    ShowChat(u->name, text, false, true);
                    ok = true;
                } else {
                    LockThing.lock();
                    for(auto o : ConnectionList) {
                        if(o->name == who) {
                            string pack = "<<WHISPER>>" + u->name + "|" + text;
                            send(o->sock, pack.c_str(), pack.length(), 0);
                            ok = true;
                        }
                    }
                    LockThing.unlock();
                }
                if(ok) {
                    string s = "<<WHISPER_SENT>>" + who + "|" + text;
                    send(u->sock, s.c_str(), s.length(), 0);
                } else {
                    send(u->sock, "<<SYS>>User not found.", 22, 0);
                }
            }
        } else {
            bool spam = false;
            if (NoSpam) {
                auto now = chrono::steady_clock::now();
                long long diff = chrono::duration_cast<chrono::milliseconds>(now - u->lastTime).count();
                if (diff < 500 || m == u->lastMsg) spam = true;
                u->lastTime = now;
                u->lastMsg = m;
            }
            if (spam) {
                send(u->sock, "<<SYS>>Spam detected. Message blocked.", 38, 0);
            } else {
                string clean = FixText(m);
                TellEveryone(u->name + ": " + clean, u->sock);
                string echo = "<<ECHO>>" + clean;
                send(u->sock, echo.c_str(), echo.length(), 0);
                ShowChat(u->name, clean, false);
            }
        }
    }
    closesocket(u->sock);
    LockThing.lock();
    for (int i=0; i<ConnectionList.size(); i++) {
        if (ConnectionList[i] == u) {
            ConnectionList.erase(ConnectionList.begin() + i);
            break;
        }
    }
    LockThing.unlock();
    delete u;
    SendUserList();
}

void HostThread(SOCKET l) {
    while (AppRunning) {
        sockaddr_in addr;
        int sz = sizeof(addr);
        SOCKET s = accept(l, (sockaddr*)&addr, &sz);
        if (s == INVALID_SOCKET) continue;
        char b[4096];
        int len = recv(s, b, 4096, 0);
        string p = "";
        if(len > 0) p = string(b, len);
        char real_p[256]; 
        GetDlgItemText(MainAppWindow, 106, real_p, 256); 
        if (!IsPublic && p != string(real_p)) {
            send(s, "INVALID", 7, 0);
            closesocket(s);
            continue;
        }
        send(s, "OK", 2, 0);
        len = recv(s, b, 4096, 0);
        string n = "Guest";
        if (len > 0) n = string(b, len);
        UserData* ud = new UserData;
        ud->sock = s;
        ud->name = n;
        ud->id = ++TotalUsers;
        ud->lastTime = chrono::steady_clock::now();
        LockThing.lock();
        ConnectionList.push_back(ud);
        LockThing.unlock();
        thread(ClientManager, ud).detach();
        SendUserList();
    }
}

void JoinThread() {
    char b[4096];
    while (AppRunning) {
        int n = recv(GlobalSock, b, 4095, 0);
        if (n <= 0) {
            ShowSys("Disconnected.");
            AppRunning = false;
            break;
        }
        b[n] = 0;
        string m = b;
        if (m.find("<<USERS>>") == 0) RefreshUsers(m.substr(9));
        else if (m.find("<<WHISPER>>") == 0) {
            string d = m.substr(11);
            int p = d.find('|');
            if (p != -1) ShowChat(d.substr(0, p), d.substr(p+1), false, true);
        }
        else if (m.find("<<WHISPER_SENT>>") == 0) {
            string d = m.substr(16);
            int p = d.find('|');
            if (p != -1) ShowChat(d.substr(0, p), d.substr(p+1), true, true);
        }
        else if (m.find("<<ECHO>>") == 0) ShowChat("You", m.substr(8), true);
        else if (m.find("<<SYS>>") == 0) ShowSys(m.substr(7));
        else {
            int s = m.find(": ");
            if (s != -1) ShowChat(m.substr(0, s), m.substr(s + 2), false);
        }
    }
    EnableWindow(TypeBox, FALSE);
    EnableWindow(SendBtn, FALSE);
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

void HandleSend(HWND h, string msg) {
    if (msg == "") return;
    if (msg == "/p") {
        IsWhispering = false;
        SetWindowText(SmallLabel, "Public Chat");
        SetDlgItemText(h, 102, "");
        return;
    }
    if (msg.find("/w ") == 0 && msg.length() > 3) {
        TargetName = msg.substr(3);
        IsWhispering = true;
        SetWindowText(SmallLabel, ("PRIVATE MODE [To: " + TargetName + "]").c_str());
        SetDlgItemText(h, 102, "");
        return;
    }
    string out = msg;
    if (IsWhispering) out = "/w " + TargetName + " " + msg;
    if (HostingNow) {
        if (out.find("/w ") == 0) {
            int gap = out.find(' ', 3);
            if (gap != -1) {
                string t = out.substr(3, gap-3);
                string m = out.substr(gap+1);
                bool ok = false;
                LockThing.lock();
                for(auto c : ConnectionList) {
                    if(c->name == t) {
                        string p = "<<WHISPER>>" + MyName + "|" + m;
                        send(c->sock, p.c_str(), p.length(), 0);
                        ShowChat(t, m, true, true);
                        ok = true; 
                        break;
                    }
                }
                LockThing.unlock();
                if(!ok) ShowSys("User " + t + " not found.");
            }
        } else {
            string clean = FixText(out);
            TellEveryone(MyName + ": " + clean, INVALID_SOCKET);
            ShowChat("You", clean, true);
        }
    } else {
        send(GlobalSock, out.c_str(), out.length(), 0);
    }
    SetDlgItemText(h, 102, ""); 
}

void StartHost(string pass, string name, int port) {
    LoadWords(); 
    MyName = name;
    HostingNow = true;
    SOCKET l = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = INADDR_ANY;
    if (bind(l, (sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) {
        MessageBox(NULL, "Port busy!", "Error", MB_ICONERROR);
        return;
    }
    listen(l, SOMAXCONN);
    AppRunning = true;
    RefreshUsers(MyName);
    thread(HostThread, l).detach();
    char pbuf[20];
    sprintf(pbuf, "%d", port);
    ShowSys("Party Started on Port " + string(pbuf) + (IsPublic ? " [Public]" : " [Private]"));
}

void StartJoin(string ip, string pass, string name, int port) {
    MyName = name;
    HostingNow = false;
    SOCKET c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &a.sin_addr);
    if (connect(c, (sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) {
        MessageBox(NULL, "No connect.", "Error", MB_ICONERROR);
        return;
    }
    send(c, pass.c_str(), pass.length(), 0);
    char buf[1024];
    int len = recv(c, buf, 1024, 0); 
    if (len <= 0 || string(buf, len) != "OK") {
        MessageBox(NULL, "Bad Pass", "Error", MB_ICONERROR);
        closesocket(c);
        return;
    }
    send(c, MyName.c_str(), MyName.length(), 0);
    GlobalSock = c;
    AppRunning = true;
    thread(JoinThread).detach();
    ShowSys("Joined Party!");
}

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