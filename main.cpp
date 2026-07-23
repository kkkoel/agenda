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

// ============ 帮助信息 ============

void printUsage(const char* progName) {
    cout << "========================================\n";
    cout << "        MySchedule - Task Manager\n";
    cout << "========================================\n";
    cout << "\nUSAGE:\n";
    cout << "  " << progName << " run\n";
    cout << "      Start interactive shell mode. Prompts for login/register,\n";
    cout << "      then loops waiting for commands. Runs a background thread\n";
    cout << "      that checks reminders every second.\n\n";

    cout << "  " << progName << " <username> <password> register\n";
    cout << "      Register a new account, then exit.\n\n";

    cout << "  " << progName << " <username> <password> addtask <name> <time> [priority] [category]\n";
    cout << "      Add a task, save to file, then exit.\n";
    cout << "      <time>     format: YYYY-MM-DD_HH:MM\n";
    cout << "      [priority] optional, one of High/Medium/Low, default: Medium\n";
    cout << "      [category] optional, one of Study/Entertainment/Life, default: Life\n\n";

    cout << "  " << progName << " <username> <password> showtask <date>\n";
    cout << "      Show all tasks on a given date, sorted by start time.\n";
    cout << "      <date> format: YYYY-MM-DD\n\n";

    cout << "  " << progName << " <username> <password> showall\n";
    cout << "      Show all tasks for this user.\n\n";

    cout << "  " << progName << " <username> <password> deltask <id>\n";
    cout << "      Delete a task by its id.\n\n";

    cout << "  " << progName << " --help | -h\n";
    cout << "      Show this help message.\n\n";

    cout << "EXAMPLES:\n";
    cout << "  " << progName << " run\n";
    cout << "  " << progName << " user1 password123 register\n";
    cout << "  " << progName << " user1 password123 addtask Homework 2026-07-27_10:00 High Study\n";
    cout << "  " << progName << " user1 password123 addtask \"Do Homework\" 2026-07-27_10:00\n";
    cout << "  " << progName << " user1 password123 showtask 2026-07-27\n";
    cout << "  " << progName << " user1 password123 showall\n";
    cout << "  " << progName << " user1 password123 deltask 1\n";
}

void showInteractiveHelp() {
    cout << "\nCommands:\n";
    cout << "  addtask <name> <time> [priority] [category]\n";
    cout << "  addtask \"<name with spaces>\" <time> [priority] [category]\n";
    cout << "  showtask <date>     e.g. showtask 2026-07-27\n";
    cout << "  showall\n";
    cout << "  deltask <id>        e.g. deltask 1\n";
    cout << "  quit\n";
    cout << "\nTime format: YYYY-MM-DD_HH:MM (use underscore, no spaces)\n";
    cout << "[priority] optional, default: Medium. [category] optional, default: Life.\n";
    cout << "Example: addtask Homework 2026-07-27_10:00 High Study\n";
    cout << "Example: addtask \"Do Homework\" 2026-07-27_10:00\n";
}

// ============ 后台提醒线程 ============

void reminderThreadFunc(TaskManager* manager) {
    while (g_running) {
        manager->checkReminders();
        this_thread::sleep_for(chrono::seconds(1));
    }
}

// ============ addtask 公共逻辑（交互模式和命令行模式共用） ============

bool doAddTask(TaskManager& manager, const string& name, const string& timeStr,
               const string& priority, const string& category) {
    if (name.empty() || timeStr.empty()) {
        cout << "[ERROR] Task name and time are required.\n";
        return false;
    }
    time_t startTime = parseTime(timeStr);
    if (startTime == -1) return false;
    time_t remindTime = startTime - 300; // 提前5分钟提醒

    string finalPriority = priority.empty() ? "Medium" : priority;
    string finalCategory = category.empty() ? "Life" : category;

    if (manager.addTask(name, startTime, finalPriority, finalCategory, remindTime)) {
        cout << "Task added successfully!\n";
        return true;
    } else {
        cout << "Failed to add task.\n";
        return false;
    }
}

// ============ 交互模式（run） ============

int runInteractiveMode() {
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
    showInteractiveHelp();

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

            ss >> timeStr >> priority >> category; // priority/category可能读不到,是空字符串,doAddTask会用默认值

            doAddTask(manager, name, timeStr, priority, category);
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
            showInteractiveHelp();
        } else {
            cout << "Unknown command. Type 'help'.\n";
        }
    }

    g_running = false;
    reminderThread.join();

    return 0;
}

// ============ 命令行单命令模式 ============
// myschedule <username> <password> <command> [args...]

int runSingleCommand(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc < 4) {
        cerr << "[ERROR] Missing arguments.\n\n";
        printUsage(argv[0]);
        return 1;
    }

    string username = argv[1];
    string password = argv[2];
    string command = argv[3];

    if (command == "register") {
        if (registerUser(username, password)) {
            cout << "Registration successful!\n";
            return 0;
        } else {
            cout << "Registration failed.\n";
            return 1;
        }
    }

    // 除register外，其它命令都需要先登录
    if (!loginUser(username, password)) {
        cout << "Login failed.\n";
        return 1;
    }

    TaskManager manager(username);

    if (command == "addtask") {
        // myschedule user pass addtask <name> <time> [priority] [category]
        if (argc < 6) {
            cerr << "[ERROR] Usage: " << argv[0]
                 << " <username> <password> addtask <name> <time> [priority] [category]\n";
            return 1;
        }
        string name = argv[4];
        string timeStr = argv[5];
        string priority = (argc >= 7) ? argv[6] : "";
        string category = (argc >= 8) ? argv[7] : "";

        return doAddTask(manager, name, timeStr, priority, category) ? 0 : 1;

    } else if (command == "showtask") {
        // myschedule user pass showtask <date>
        if (argc < 5) {
            cerr << "[ERROR] Usage: " << argv[0] << " <username> <password> showtask <date>\n";
            return 1;
        }
        string dateStr = argv[4];
        time_t date = parseTime(dateStr + "_00:00");
        if (date == -1) return 1;
        manager.showTasksForDay(date);
        return 0;

    } else if (command == "showall") {
        manager.showAllTasks();
        return 0;

    } else if (command == "deltask") {
        if (argc < 5) {
            cerr << "[ERROR] Usage: " << argv[0] << " <username> <password> deltask <id>\n";
            return 1;
        }
        int id = atoi(argv[4]);
        if (manager.deleteTask(id)) {
            cout << "Task deleted.\n";
            return 0;
        } else {
            return 1;
        }

    } else {
        cerr << "[ERROR] Unknown command: " << command << "\n\n";
        printUsage(argv[0]);
        return 1;
    }
}

// ============ 程序入口 ============

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    string firstArg = argv[1];

    if (firstArg == "--help" || firstArg == "-h") {
        printUsage(argv[0]);
        return 0;
    }

    if (firstArg == "run") {
        return runInteractiveMode();
    }

    // 否则按 "myschedule <user> <pass> <command> [args...]" 解析
    return runSingleCommand(argc, argv);
}