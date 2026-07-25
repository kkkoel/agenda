#include "VoiceManager.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <ctime>

using namespace std;

// 等待用户按一次回车。之前的实现需要连续按两次回车才会继续
// (第一次getchar循环消耗掉的其实就是用户按的那次回车，然后又
// 额外调用了一次getchar()等第二次输入)，这里改成只需要一次。
void waitForEnter() {
    string dummy;
    getline(cin, dummy);
}

// whisper.cpp的可执行文件和模型路径。这两个文件不属于本项目、
// 也不会被提交到git仓库里，需要使用者自行安装whisper.cpp并保持
// 这个路径可用，否则语音识别会因为找不到命令而失败。
static const string WHISPER_BIN   = "~/Desktop/whisper.cpp/build/bin/whisper-cli";
static const string WHISPER_MODEL = "~/Desktop/whisper.cpp/models/ggml-tiny.en.bin";

// 展开路径开头的~为$HOME，用于配合stat/ifstream等不会自动做shell展开的调用
static string expandHome(const string& path) {
    if (!path.empty() && path[0] == '~') {
        const char* home = getenv("HOME");
        if (home != nullptr) {
            return string(home) + path.substr(1);
        }
    }
    return path;
}

// 检查语音识别所需的外部依赖(arecord、whisper.cpp可执行文件、模型文件)是否就绪。
// 找不到时打印清晰的诊断信息，而不是让后续步骤静默失败、只给一句「识别失败」。
bool voiceDependenciesReady() {
    bool ok = true;

    if (system("command -v arecord >/dev/null 2>&1") != 0) {
        cerr << "[VOICE] 'arecord' not found. Install it with: sudo apt install alsa-utils\n";
        ok = false;
    }

    ifstream binCheck(expandHome(WHISPER_BIN));
    if (!binCheck.good()) {
        cerr << "[VOICE] whisper-cli not found at: " << WHISPER_BIN << "\n";
        cerr << "        Build whisper.cpp first, or edit WHISPER_BIN in VoiceManager.cpp\n";
        cerr << "        to match where it's installed on this machine.\n";
        ok = false;
    }

    ifstream modelCheck(expandHome(WHISPER_MODEL));
    if (!modelCheck.good()) {
        cerr << "[VOICE] whisper model not found at: " << WHISPER_MODEL << "\n";
        cerr << "        Download a ggml model into whisper.cpp/models/, or edit\n";
        cerr << "        WHISPER_MODEL in VoiceManager.cpp to match this machine.\n";
        ok = false;
    }

    return ok;
}

bool recordAudio(const string& filename, int seconds) {
    cout << "  Press Enter to start recording (" << seconds << " seconds)...\n";
    waitForEnter();

    cout << "  Recording... (" << seconds << " seconds)\n";
    string cmd = "arecord -f cd -d " + to_string(seconds) + " -t wav " + filename + " 2>/tmp/voice_record_err.log";
    int status = system(cmd.c_str());
    if (status != 0) {
        cerr << "[VOICE] Recording failed. Run 'arecord -l' to check your microphone,\n";
        cerr << "        or see /tmp/voice_record_err.log for details.\n";
        return false;
    }

    cout << "  Recording finished.\n";
    return true;
}

string recognizeSpeech(const string& filename) {
    string whisperCmd = WHISPER_BIN + " -f " + filename +
                        " -m " + WHISPER_MODEL +
                        " --no-timestamps 2>/tmp/voice_recognize_err.log > /tmp/voice.txt";
    int status = system(whisperCmd.c_str());

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

    if (result.empty()) {
        cerr << "[VOICE] Could not recognize any speech"
             << (status != 0 ? " (whisper-cli exited with an error)" : "")
             << ".\n";
        cerr << "        See /tmp/voice_recognize_err.log for details.\n";
    }

    return result;
}

string parseDateFromText(const string& text) {
    string digits;
    for (char c : text) {
        if (isdigit(c)) digits += c;
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
        if (isdigit(c)) digits += c;
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