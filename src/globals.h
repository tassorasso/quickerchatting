#pragma once
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

#ifndef EM_SHOWSCROLLBAR
#define EM_SHOWSCROLLBAR (WM_USER + 96)
#endif

struct UserData {
    SOCKET sock;
    string name;
    int id;
    string lastMsg;
    chrono::steady_clock::time_point lastTime;
};

extern HWND MainAppWindow;
extern HWND ChatArea;
extern HWND TypeBox;
extern HWND SendBtn;
extern HWND PinBox;
extern HWND UsersList;
extern HWND SmallLabel;
extern HWND MemberLabel;
extern HWND OptButton;
extern HWND PublicCheck;
extern HWND PassBox;
extern HWND PortBox;

extern SOCKET GlobalSock;
extern bool AppRunning;
extern string MyName;
extern bool HostingNow;
extern bool AmIConnected;
extern bool IsPublic;
extern bool Invisible;

extern COLORREF color_bg;
extern COLORREF color_trans;
extern COLORREF color_me;
extern COLORREF color_them;
extern COLORREF color_sys;
extern COLORREF color_private;
extern COLORREF color_text;

extern bool IsWhispering;
extern string TargetName;
extern bool BadWordFilter;
extern bool NoSpam;

extern vector<string> ListOfBadWords;
extern vector<UserData*> ConnectionList;
extern mutex LockThing;
extern int TotalUsers;

extern HFONT f1, f2, f3;
extern HBRUSH BackgroundBrush;