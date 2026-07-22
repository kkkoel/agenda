#include "account_system.h"
#include "TaskManager.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <string>
#include <cstring>
#include <cctype>
#include <windows.h>

using namespace std;

// 归一化：把常见的全角符号转换成半角，防止中文输入法导致解析失败
static void normalizePunctuation(string& s) {
    // UTF-8 三字节全角符号 -> 半角ASCII
    // － (U+FF0D) -> -
    // ＿ (U+FF3F) -> _
    // ： (U+FF1A) -> :
    // 　(U+3000,全角空格) -> 普通空格
    struct Pair { const char* full; char half; };
    static const Pair table[] = {
        {"\xEF\xBC\x8D", '-'},
        {"\xEF\xBC\xBF", '_'},
        {"\xEF\xBC\x9A", ':'},
        {"\xE3\x80\x80", ' '},
    };
    for (auto& p : table) {
        size_t pos;
        while ((pos = s.find(p.full)) != string::npos) {
            s.replace(pos, strlen(p.full), 1, p.half);
        }
    }
}

time_t parseTime(const string& rawTimeStr) {
    string timeStr = rawTimeStr;
    normalizePunctuation(timeStr);

    // 去除首尾空白（防止多打了空格）
    while (!timeStr.empty() && isspace((unsigned char)timeStr.front())) timeStr.erase(0, 1);
    while (!timeStr.empty() && isspace((unsigned char)timeStr.back()))  timeStr.pop_back();

    int year, month, day, hour, min;
    int matched = sscanf(timeStr.c_str(), "%d-%d-%d_%d:%d", &year, &month, &day, &hour, &min);

    if (matched != 5) {
        cerr << "[ERROR] Invalid time format: \"" << timeStr << "\"\n";
        cerr << "       Use: YYYY-MM-DD_HH:MM (e.g. 2026-07-27_10:00)\n";
        // 调试用：打印每个字符的字节值，方便确认是否混入了全角符号
        cerr << "       [DEBUG] bytes: ";
        for (unsigned char ch : timeStr) cerr << (int)ch << " ";
        cerr << "\n";
        return -1;
    }

    if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || min < 0 || min > 59) {
        cerr << "[ERROR] Time values out of range: " << timeStr << "\n";
        return -1;
    }

    tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    time_t result = mktime(&tm);
    if (result == -1) {
        cerr << "[ERROR] mktime failed for: " << timeStr << "\n";
    }
    return result;
}

string getCurrentTimeStr() {
    time_t now = time(nullptr);
    tm* local = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", local);
    return string(buf);
}

void showHelp() {
    cout << "\nCommands:\n";
    cout << "  addtask <name> <time> <priority> <category>\n";
    cout << "  addtask \"<name with spaces>\" <time> <priority> <category>\n";
    cout << "  showtask <date>     e.g. showtask 2026-07-27\n";
    cout << "  showall\n";
    cout << "  deltask <id>        e.g. deltask 1\n";
    cout << "  quit\n";
    cout << "\nTime format: YYYY-MM-DD_HH:MM (use underscore, no spaces)\n";
    cout << "Example: addtask Homework 2026-07-27_10:00 High Study\n";
    cout << "Example: addtask \"Do Homework\" 2026-07-27_10:00 High Study\n";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    string username, password;
    int choice;

    cout << "========================================\n";
    cout << "      Welcome to Schedule Manager\n";
    cout << "========================================\n";
    cout << "1. Register\n";
    cout << "2. Login\n";
    cout << "Choose (1 or 2): ";
    cin >> choice;

    if (choice == 1) {
        cout << "Username: ";
        cin >> username;
        cout << "Password: ";
        cin >> password;
        if (registerUser(username, password)) {
            cout << "Registration successful!\n";
        } else {
            cout << "Registration failed.\n";
            return 1;
        }
    } else if (choice == 2) {
        cout << "Username: ";
        cin >> username;
        cout << "Password: ";
        cin >> password;
        if (!loginUser(username, password)) {
            cout << "Login failed.\n";
            return 1;
        }
        cout << "Login successful! Welcome " << username << "!\n";
    } else {
        cout << "Invalid choice.\n";
        return 1;
    }

    TaskManager manager(username);
    cout << "Current time: " << getCurrentTimeStr() << "\n";
    showHelp();

    string line;
    cin.ignore();

    while (true) {
        cout << "\n> ";
        getline(cin, line);
        if (line == "quit" || line == "exit") break;

        stringstream ss(line);
        string cmd;
        ss >> cmd;

        if (cmd == "addtask") {
            string name, timeStr, priority, category;

            ss >> ws; // 跳过前导空白
            if (ss.peek() == '"') {
                ss.get(); // 吃掉开头的引号
                getline(ss, name, '"'); // 读到下一个引号为止，支持带空格的任务名
            } else {
                ss >> name; // 不带引号时，只能是单个单词
            }

            ss >> timeStr >> priority >> category;

            if (name.empty() || timeStr.empty() || priority.empty() || category.empty()) {
                cout << "Usage: addtask <name> <time> <priority> <category>\n";
                cout << "       or with quotes: addtask \"Do Homework\" 2026-07-27_10:00 High Study\n";
                cout << "Example: addtask Homework 2026-07-27_10:00 High Study\n";
                continue;
            }

            time_t startTime = parseTime(timeStr);
            if (startTime == -1) continue;
            time_t remindTime = startTime - 300;

            if (manager.addTask(name, startTime, priority, category, remindTime)) {
                cout << "Task added successfully!\n";
            } else {
                cout << "Failed to add task.\n";
            }
        } else if (cmd == "showtask") {
            string dateStr;
            ss >> dateStr;
            time_t date = parseTime(dateStr + "_00:00");
            if (date != -1) {
                manager.showTasksForDay(date);
            }
        } else if (cmd == "showall") {
            manager.showAllTasks();
        } else if (cmd == "deltask") {
            int id;
            ss >> id;
            if (manager.deleteTask(id)) {
                cout << "Task deleted.\n";
            }
        } else if (cmd == "help") {
            showHelp();
        } else {
            cout << "Unknown command. Type 'help'.\n";
        }
    }
    return 0;
}