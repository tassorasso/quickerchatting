#include "globals.h"

HWND MainAppWindow = NULL;
HWND ChatArea = NULL;
HWND TypeBox = NULL;
HWND SendBtn = NULL;
HWND PinBox = NULL;
HWND UsersList = NULL;
HWND SmallLabel = NULL;
HWND MemberLabel = NULL;
HWND OptButton = NULL;
HWND PublicCheck = NULL;
HWND PassBox = NULL;
HWND PortBox = NULL;

SOCKET GlobalSock = INVALID_SOCKET;
bool AppRunning = false;
string MyName = "";
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
vector<UserData*> ConnectionList;
mutex LockThing;
int TotalUsers = 0;

HFONT f1, f2, f3;
HBRUSH BackgroundBrush;