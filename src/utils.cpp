#include "utils.h"

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
            }
            p = low.find(w, p+1);
        }
    }
    return input;
}