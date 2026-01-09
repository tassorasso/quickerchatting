#pragma once
#include "globals.h"

void TellEveryone(string msg, SOCKET skip);
void SendUserList();
void ClientManager(UserData* u);
void HostThread(SOCKET l);
void JoinThread();
void HandleSend(HWND h, string msg);
void StartHost(string pass, string name, int port);
void StartJoin(string ip, string pass, string name, int port);