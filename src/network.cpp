#include "network.h"
#include "utils.h"
#include "ui.h"

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