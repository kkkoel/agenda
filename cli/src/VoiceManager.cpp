#include "VoiceManager.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <ctime>

using namespace std;

void waitForEnter() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

bool recordAudio(const string& filename, int seconds) {
    cout << "  Press Enter to start recording (" << seconds << " seconds)...\n";
    waitForEnter();

    cout << "  Recording... (" << seconds << " seconds)\n";
    string cmd = "arecord -f cd -d " + to_string(seconds) + " -t wav " + filename + " 2>/dev/null";
    system(cmd.c_str());

    cout << "  Recording finished.\n";
    return true;
}

string recognizeSpeech(const string& filename) {
    string whisperCmd = "~/Desktop/whisper.cpp/build/bin/whisper-cli -f " + filename +
                        " -m ~/Desktop/whisper.cpp/models/ggml-tiny.en.bin --no-timestamps 2>/dev/null > /tmp/voice.txt";
    system(whisperCmd.c_str());

    ifstream fin("/tmp/voice.txt");
    string line, result;
    while (getline(fin, line)) {
        if (!line.empty()) {
            result = line;
            break;
        }
    }
    fin.close();

    while (!result.empty() && result.front() == ' ') result.erase(0, 1);
    while (!result.empty() && result.back() == ' ') result.pop_back();

    return result;
}

string parseDateFromText(const string& text) {
    string digits;
    for (char c : text) {
        if (c >= '0' && c <= '9') digits += c;
    }

    if (digits.length() == 8) {
        int year = stoi(digits.substr(0, 4));
        int month = stoi(digits.substr(4, 2));
        int day = stoi(digits.substr(6, 2));
        if (year >= 2024 && year <= 2030 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
            return string(buf);
        }
    }

    time_t now = time(nullptr);
    tm* tmNow = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", tmNow);
    return string(buf);
}

string parseTimeFromText(const string& text) {
    string digits;
    for (char c : text) {
        if (c >= '0' && c <= '9') digits += c;
    }

    if (digits.length() >= 4) {
        int hour = stoi(digits.substr(0, 2));
        int min = stoi(digits.substr(2, 2));
        if (hour >= 0 && hour <= 23 && min >= 0 && min <= 59) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:%02d", hour, min);
            return string(buf);
        }
    }

    return "09:00";
}

void parsePriorityAndCategory(const string& text, string& priority, string& category) {
    string lower = text;
    for (char& c : lower) c = tolower(c);

    priority = "Medium";
    category = "Life";

    if (lower.find("high") != string::npos) priority = "High";
    else if (lower.find("medium") != string::npos) priority = "Medium";
    else if (lower.find("low") != string::npos) priority = "Low";

    if (lower.find("study") != string::npos) category = "Study";
    else if (lower.find("entertainment") != string::npos) category = "Entertainment";
    else if (lower.find("life") != string::npos) category = "Life";
    else if (lower.find("work") != string::npos) category = "Life";
}
