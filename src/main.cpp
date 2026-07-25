#include "CommandHandler.h"
#include "account_system.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cctype>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>   // _mkdir
#else
#include <sys/stat.h> // mkdir
#endif

using namespace std;

atomic<bool> g_running(true);

// 确保data/目录存在(用户数据、任务数据都存在这个目录下)。
// 全新clone仓库或换一台机器运行时，这个目录不会自动存在，
// 必须在程序启动时主动创建，否则第一次注册/建任务就会因为
// 无法打开文件而失败。目录已存在时直接忽略返回值即可。
static void ensureDataDirectory() {
#ifdef _WIN32
    _mkdir("data");
#else
    mkdir("data", 0755);
#endif
}

void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void normalizePunctuation(string& s) {
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
    while (!timeStr.empty() && isspace((unsigned char)timeStr.back())) timeStr.pop_back();

    int year, month, day, hour, min;
    int matched = sscanf(timeStr.c_str(), "%d-%d-%d_%d:%d", &year, &month, &day, &hour, &min);

    if (matched != 5) {
        cerr << "[ERROR] Invalid time format: \"" << timeStr << "\"\n";
        cerr << "        Expected format: YYYY-MM-DD_HH:MM  (e.g. 2026-07-27_10:00)\n";
        return -1;
    }

    if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || min < 0 || min > 59) {
        cerr << "[ERROR] Time value out of range: \"" << timeStr << "\"\n";
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
        cerr << "[ERROR] Failed to interpret time: \"" << timeStr << "\"\n";
    }
    return result;
}

string getCurrentTimeStr() {
    time_t now = time(nullptr);
    tm* local = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M", local);
    return string(buf);
}

void printBanner(const string& subtitle) {
    cout << "+----------------------------------------------------------+\n";
    cout << "|                                                          |\n";
    cout << "|                       MYSCHEDULE                         |\n";
    cout << "|                    " << left << setw(38) << subtitle << "|\n";
    cout << "|                                                          |\n";
    cout << "+----------------------------------------------------------+\n";
}

void printUsage(const char* progName) {
    printBanner("Command-Line Task Manager");
    cout << "\nUSAGE\n";
    cout << "  " << progName << " run\n";
    cout << "        Start interactive shell mode.\n\n";

    cout << "  " << progName << " <username> <password> register\n";
    cout << "        Register a new account, then exit.\n\n";

    cout << "  " << progName << " <username> <password> addtask <n> <time> [priority] [category]\n";
    cout << "        Add one task, save it to file, then exit.\n";
    cout << "          <n>     task name (use quotes if it has spaces)\n";
    cout << "          <time>     format YYYY-MM-DD_HH:MM\n";
    cout << "          [priority] High | Medium | Low         (default: Medium)\n";
    cout << "          [category] Study | Entertainment | Life (default: Life)\n\n";

    cout << "  " << progName << " <username> <password> showtask <date>\n";
    cout << "        Show all tasks on a given date, sorted by start time.\n";
    cout << "          <date>     format YYYY-MM-DD\n\n";

    cout << "  " << progName << " <username> <password> showall\n";
    cout << "        Show every task belonging to this user.\n\n";

    cout << "  " << progName << " <username> <password> deltask <id>\n";
    cout << "        Delete one task by its numeric ID.\n\n";

    cout << "  " << progName << " --help | -h\n";
    cout << "        Show this help message.\n\n";

    cout << "EXAMPLES\n";
    cout << "  " << progName << " run\n";
    cout << "  " << progName << " alice secret123 register\n";
    cout << "  " << progName << " alice secret123 addtask Homework 2026-07-27_10:00 High Study\n";
    cout << "  " << progName << " alice secret123 addtask \"Movie night\" 2026-07-27_20:00 Low Entertainment\n";
    cout << "  " << progName << " alice secret123 showtask 2026-07-27\n";
    cout << "  " << progName << " alice secret123 showall\n";
    cout << "  " << progName << " alice secret123 deltask 3\n";
}

void showInteractiveHelp() {
    cout << "\nCOMMANDS\n";
    cout << "  " << left << setw(46) << "addtask <n> <time> [pri] [cat]" << "add a task\n";
    cout << "  " << left << setw(46) << "addtask \"<name with spaces>\" <time> ..." << "(same, quoted name)\n";
    cout << "  " << left << setw(46) << "showtask <YYYY-MM-DD>" << "list tasks on a date\n";
    cout << "  " << left << setw(46) << "showall" << "list all your tasks\n";
    cout << "  " << left << setw(46) << "deltask <id>" << "delete a task by ID\n";
    cout << "  " << left << setw(46) << "voice" << "add task by voice (Linux only)\n";
    cout << "  " << left << setw(46) << "help" << "show this help again\n";
    cout << "  " << left << setw(46) << "quit / exit" << "leave the program\n";
    cout << "\nNOTES\n";
    cout << "  - Time format:  YYYY-MM-DD_HH:MM   (underscore between date and time)\n";
    cout << "  - [priority]  High | Medium | Low          default: Medium\n";
    cout << "  - [category]  Study | Entertainment | Life default: Life\n";
    cout << "  - A task's name + start time must be unique.\n";
    cout << "\nEXAMPLES\n";
    cout << "  addtask Homework 2026-07-27_10:00 High Study\n";
    cout << "  addtask \"Movie night\" 2026-07-27_20:00\n";
}

bool doAddTask(TaskManager& manager, const string& name, const string& timeStr,
               const string& priority, const string& category) {
    if (name.empty()) {
        cerr << "[ERROR] Task name is required.\n";
        return false;
    }

    string finalTimeStr = timeStr;
    if (finalTimeStr.empty()) {
        time_t now = time(nullptr);
        time_t later = now + 3600;
        tm* tmNow = localtime(&later);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M", tmNow);
        finalTimeStr = string(buf);
    }

    time_t startTime = parseTime(finalTimeStr);
    if (startTime == -1) return false;
    time_t remindTime = startTime - 300;

    string finalPriority = priority.empty() ? "Medium" : priority;
    string finalCategory = category.empty() ? "Life" : category;

    return manager.addTask(name, startTime, finalPriority, finalCategory, remindTime);
}

int main(int argc, char* argv[]) {
    setupConsole();
    ensureDataDirectory();

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

    return runSingleCommand(argc, argv);
}