#pragma once
#include "globals.h"

void FlashTaskbar();
void SelectColor(HWND h, COLORREF& target);
void ToggleUI(bool v);
void SetGhost(bool mode);
void PushText(string t, COLORREF c, bool b);
void ShowSys(string m);
void ShowChat(string user, string m, bool me, bool w = false);
void RefreshUsers(string raw);
void BuildChatUI(HWND h);