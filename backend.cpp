#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

struct Client { SOCKET s; string name; };

SOCKET g_sock = INVALID_SOCKET;
bool running = false;
string me = "";
bool hosting = false;

bool g_no_spam = true;
bool g_pin = false;

vector<Client*> clients;
mutex mtx;
queue<pair<string, string>> events;
mutex ev_mtx;
string last_msg = "";

void add_event(string t, string d) {
    lock_guard<mutex> k(ev_mtx);
    events.push({t, d});
}

void broadcast(string msg, SOCKET ignore) {
    lock_guard<mutex> k(mtx);
    for (auto c : clients) {
        if (c->s != ignore) send(c->s, msg.c_str(), msg.length(), 0);
    }
}

void sync_users() {
    string d = me;
    lock_guard<mutex> k(mtx);
    for (auto c : clients) d += "," + c->name;
    add_event("USERS", d);
    if (hosting) {
        string net_d = "<<USERS>>" + d;
        for (auto c : clients) send(c->s, net_d.c_str(), net_d.length(), 0);
    }
}

void handle_client(Client* c) {
    char buf[4096];
    while (running) {
        int r = recv(c->s, buf, 4095, 0);
        if (r <= 0) break;
        buf[r] = 0;
        string msg = buf;
        
        broadcast(c->name + ": " + msg, c->s);
        string echo = "<<ECHO>>" + msg;
        send(c->s, echo.c_str(), echo.length(), 0);
        add_event("CHAT", c->name + ": " + msg);
    }
    closesocket(c->s);
    mtx.lock();
    auto it = find(clients.begin(), clients.end(), c);
    if (it != clients.end()) clients.erase(it);
    mtx.unlock();
    delete c;
    sync_users();
}

void host_loop(SOCKET listener) {
    while (running) {
        sockaddr_in a;
        int sz = sizeof(a);
        SOCKET s = accept(listener, (sockaddr*)&a, &sz);
        if (s == INVALID_SOCKET) continue;
        char b[4096];
        send(s, "OK", 2, 0); 
        int len = recv(s, b, 4096, 0);
        string n = (len > 0) ? string(b, len) : "Guest";
        Client* c = new Client{s, n};
        mtx.lock();
        clients.push_back(c);
        mtx.unlock();
        thread(handle_client, c).detach();
        sync_users();
    }
}

void join_loop() {
    char b[4096];
    while (running) {
        int n = recv(g_sock, b, 4095, 0);
        if (n <= 0) {
            add_event("SYS", "Disconnected.");
            running = false;
            break;
        }
        b[n] = 0;
        string m = b;
        if (m.find("<<USERS>>") == 0) add_event("USERS", m.substr(9));
        else if (m.find("<<ECHO>>") == 0) add_event("CHAT", me + ": " + m.substr(8));
        else add_event("CHAT", m);
    }
}

extern "C" {
    __declspec(dllexport) void init() {
        WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
        running = true;
    }

    __declspec(dllexport) void start_host(int port, const char* name) {
        me = name; hosting = true;
        SOCKET l = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in a = {0};
        a.sin_family = AF_INET; a.sin_port = htons(port); a.sin_addr.s_addr = INADDR_ANY;
        if (bind(l, (sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) { add_event("ERR", "Port busy"); return; }
        listen(l, SOMAXCONN);
        add_event("SYS", "Hosting on " + to_string(port));
        thread(host_loop, l).detach();
        sync_users();
    }

    __declspec(dllexport) void start_join(const char* ip, int port, const char* name) {
        me = name; hosting = false;
        g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in a = {0};
        a.sin_family = AF_INET; a.sin_port = htons(port);
        inet_pton(AF_INET, ip, &a.sin_addr);
        if (connect(g_sock, (sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) { add_event("ERR", "Connect failed"); return; }
        char d[10]; recv(g_sock, d, 10, 0); 
        send(g_sock, name, strlen(name), 0);
        add_event("SYS", "Connected to " + string(ip));
        thread(join_loop).detach();
    }

    __declspec(dllexport) void send_msg(const char* msg) {
        string m = msg;
        if (g_no_spam && m == last_msg) return; 
        last_msg = m;
        if (hosting) {
            broadcast(me + ": " + m, INVALID_SOCKET);
            add_event("CHAT", me + ": " + m);
        } else {
            send(g_sock, m.c_str(), m.length(), 0);
        }
    }

    __declspec(dllexport) int poll(char* t, char* d) {
        lock_guard<mutex> k(ev_mtx);
        if (events.empty()) return 0;
        auto e = events.front(); events.pop();
        strcpy(t, e.first.c_str()); strcpy(d, e.second.c_str());
        return 1;
    }
    
    
    __declspec(dllexport) void set_spam(bool on) { g_no_spam = on; }
    __declspec(dllexport) void kill() { running = false; closesocket(g_sock); WSACleanup(); }
}