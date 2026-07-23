#include "account_system.h"
#include "TaskManager.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <atomic>
#include <chrono>
#include <windows.h>

using namespace std;

// 控制后台提醒线程的运行状态，主线程退出前会置为false并join
atomic<bool> g_running(true);

static void normalizePunctuation(string& s) {
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

    while (!timeStr.empty() && isspace((unsigned char)timeStr.front())) timeStr.erase(0, 1);
    while (!timeStr.empty() && isspace((unsigned char)timeStr.back()))  timeStr.pop_back();

    int year, month, day, hour, min;
    int matched = sscanf(timeStr.c_str(), "%d-%d-%d_%d:%d", &year, &month, &day, &hour, &min);

    if (matched != 5) {
        cerr << "[ERROR] Invalid time format: \"" << timeStr << "\"\n";
        cerr << "       Use: YYYY-MM-DD_HH:MM (e.g. 2026-07-27_10:00)\n";
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

// 后台线程函数：每隔1秒检查一次是否有任务到了提醒时间
void reminderThreadFunc(TaskManager* manager) {
    while (g_running) {
        manager->checkReminders();
        this_thread::sleep_for(chrono::seconds(1));
    }
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

    // 启动后台提醒线程，独立于用户输入循环运行
    thread reminderThread(reminderThreadFunc, &manager);

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

            ss >> ws;
            if (ss.peek() == '"') {
                ss.get();
                getline(ss, name, '"');
            } else {
                ss >> name;
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

    // 通知后台线程退出，并等待它结束，避免程序退出时线程还在跑导致崩溃
    g_running = false;
    reminderThread.join();

    return 0;
}