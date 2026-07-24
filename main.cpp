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
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <limits>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

atomic<bool> g_running(true);

static void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

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

static void printBanner(const string& subtitle) {
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

    cout << "  " << progName << " <username> <password> addtask <name> <time> [priority] [category]\n";
    cout << "        Add one task, save it to file, then exit.\n";
    cout << "          <name>     task name (use quotes if it has spaces)\n";
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

static void showInteractiveHelp() {
    cout << "\nCOMMANDS\n";
    cout << "  " << left << setw(46) << "addtask <name> <time> [pri] [cat]" << "add a task\n";
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

void reminderThreadFunc(TaskManager* manager) {
    while (g_running) {
        manager->checkReminders();
        this_thread::sleep_for(chrono::seconds(1));
    }
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

void waitForEnter() {
    // 清空 stdin 中所有残留字符（包括换行符）
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    // 然后再等待一个新的 Enter
    getchar();
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

int runInteractiveMode() {
    setupConsole();
    printBanner("Interactive Mode");

    string username, password;
    int choice;

    cout << "\n  1) Register\n";
    cout << "  2) Login\n";
    cout << "\nChoose (1 or 2): ";
    cin >> choice;

    if (choice == 1) {
        cout << "Username: ";
        cin >> username;
        cout << "Password: ";
        cin >> password;
        if (registerUser(username, password)) {
            cout << "\n[OK] Registration successful. You are now logged in as \"" << username << "\".\n";
        } else {
            cout << "\n[FAILED] Registration failed.\n";
            return 1;
        }
    } else if (choice == 2) {
        cout << "Username: ";
        cin >> username;
        cout << "Password: ";
        cin >> password;
        if (!loginUser(username, password)) {
            cout << "\n[FAILED] Login failed.\n";
            return 1;
        }
        cout << "\n[OK] Login successful. Welcome back, " << username << "!\n";
    } else {
        cout << "\n[ERROR] Invalid choice.\n";
        return 1;
    }

    TaskManager manager(username);
    cout << "Server time: " << getCurrentTimeStr() << "\n";
    showInteractiveHelp();

    thread reminderThread(reminderThreadFunc, &manager);

    string line;
    cin.ignore();

    while (true) {
        cout << "\n[" << username << "] > ";
        getline(cin, line);
        if (line == "quit" || line == "exit") break;
        if (line.empty()) continue;

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

            doAddTask(manager, name, timeStr, priority, category);
        } else if (cmd == "showtask") {
            string dateStr;
            ss >> dateStr;
            if (dateStr.empty()) {
                cerr << "[ERROR] Usage: showtask <YYYY-MM-DD>\n";
                continue;
            }
            time_t date = parseTime(dateStr + "_00:00");
            if (date != -1) {
                manager.showTasksForDay(date);
            }
        } else if (cmd == "showall") {
            manager.showAllTasks();
        } else if (cmd == "deltask") {
            int id;
            if (!(ss >> id)) {
                cerr << "[ERROR] Usage: deltask <id>\n";
                continue;
            }
            manager.deleteTask(id);
        } else if (cmd == "voice") {
#ifdef __linux__
            cout << "\n----------------------------------------------\n";
            cout << "  VOICE TASK ENTRY\n";
            cout << "----------------------------------------------\n";
            cout << "  You will be guided step by step.\n";
            cout << "----------------------------------------------\n\n";

            string taskName, dateStr, timeStr, priority, category;
            string recognized;

            cout << "[1/4] Say the task name (e.g. write report) - 5 seconds\n";
            if (!recordAudio("/tmp/voice.wav", 5)) continue;
            recognized = recognizeSpeech("/tmp/voice.wav");
            if (recognized.empty()) {
                cout << "[FAILED] Recognition failed. Try again.\n";
                continue;
            }
            taskName = recognized;
            cout << "[OK] Recognized: \"" << taskName << "\"\n\n";

            cout << "[2/4] Say 8 digits for date (e.g. 20260724) - 10 seconds\n";
            cout << "      say: two zero two six zero seven two four\n";
            if (!recordAudio("/tmp/voice.wav", 10)) continue;
            recognized = recognizeSpeech("/tmp/voice.wav");
            if (recognized.empty()) {
                cout << "[FAILED] Recognition failed. Try again.\n";
                continue;
            }
            dateStr = parseDateFromText(recognized);
            cout << "[OK] Recognized: \"" << recognized << "\" -> " << dateStr << "\n\n";

            cout << "[3/4] Say 4 digits for time (e.g. 1430) - 5 seconds\n";
            cout << "      say: one four three zero\n";
            if (!recordAudio("/tmp/voice.wav", 5)) continue;
            recognized = recognizeSpeech("/tmp/voice.wav");
            if (recognized.empty()) {
                cout << "[FAILED] Recognition failed. Try again.\n";
                continue;
            }
            timeStr = parseTimeFromText(recognized);
            cout << "[OK] Recognized: \"" << recognized << "\" -> " << timeStr << "\n\n";

            cout << "[4/4] Say priority and category: high study - 5 seconds\n";
            cout << "      (priority: high / medium / low, category: study / entertainment / life)\n";
            if (!recordAudio("/tmp/voice.wav", 5)) continue;
            recognized = recognizeSpeech("/tmp/voice.wav");
            if (recognized.empty()) {
                cout << "[FAILED] Recognition failed. Try again.\n";
                continue;
            }
            parsePriorityAndCategory(recognized, priority, category);
            cout << "[OK] Recognized: \"" << recognized << "\" -> " << priority << " / " << category << "\n\n";

            string fullTime = dateStr + "_" + timeStr;

            cout << "----------------------------------------------\n";
            cout << "  PARSED TASK\n";
            cout << "----------------------------------------------\n";
            cout << "  Name:     " << taskName << "\n";
            cout << "  Time:     " << fullTime << "\n";
            cout << "  Priority: " << priority << "\n";
            cout << "  Category: " << category << "\n";
            cout << "----------------------------------------------\n";

            string confirm;
            cout << "  Confirm? (y/n/edit): ";
            getline(cin, confirm);

            if (confirm == "n" || confirm == "N") {
                cout << "[INFO] Task creation cancelled.\n";
                continue;
            } else if (confirm == "edit" || confirm == "e") {
                cout << "  Edit name (current: " << taskName << "): ";
                getline(cin, taskName);
                if (taskName.empty()) taskName = "Untitled";

                cout << "  Edit date (current: " << dateStr << "): ";
                getline(cin, dateStr);
                if (dateStr.empty()) dateStr = parseDateFromText("");

                cout << "  Edit time (current: " << timeStr << "): ";
                getline(cin, timeStr);
                if (timeStr.empty()) timeStr = "09:00";

                cout << "  Edit priority (current: " << priority << "): ";
                getline(cin, priority);
                if (priority.empty()) priority = "Medium";

                cout << "  Edit category (current: " << category << "): ";
                getline(cin, category);
                if (category.empty()) category = "Life";

                fullTime = dateStr + "_" + timeStr;
            }

            doAddTask(manager, taskName, fullTime, priority, category);
#else
            cout << "[ERROR] Voice command is only supported on Linux.\n";
#endif
        } else if (cmd == "help") {
            showInteractiveHelp();
        } else {
            cout << "[ERROR] Unknown command \"" << cmd << "\". Type 'help' for a list of commands.\n";
        }
    }

    g_running = false;
    reminderThread.join();

    cout << "\nGoodbye, " << username << "!\n";
    return 0;
}

int runSingleCommand(int argc, char* argv[]) {
    setupConsole();

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
            cout << "[OK] Registration successful for \"" << username << "\".\n";
            return 0;
        } else {
            cerr << "[FAILED] Registration failed.\n";
            return 1;
        }
    }

    if (!loginUser(username, password)) {
        cerr << "[FAILED] Login failed for \"" << username << "\".\n";
        return 1;
    }

    TaskManager manager(username);

    if (command == "addtask") {
        if (argc < 6) {
            cerr << "[ERROR] Missing arguments for addtask.\n";
            cerr << "        Usage: " << argv[0] << " <username> <password> addtask <name> <time> [priority] [category]\n";
            cerr << "        Example: " << argv[0] << " test 123456 addtask Homework 2026-07-27_10:00 High Study\n";
            cerr << "        Note: For names with spaces, use quotes: \"Movie Night\"\n";
            return 1;
        }
        string name = argv[4];
        if (name.front() == '"') {
            name = name.substr(1);
            for (int i = 5; i < argc; ++i) {
                string part = argv[i];
                if (part.back() == '"') {
                    name += " " + part.substr(0, part.size() - 1);
                    break;
                } else {
                    name += " " + part;
                }
            }
        }
        string timeStr = argv[5];
        string priority = (argc >= 7) ? argv[6] : "";
        string category = (argc >= 8) ? argv[7] : "";

        return doAddTask(manager, name, timeStr, priority, category) ? 0 : 1;

    } else if (command == "showtask") {
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
        return manager.deleteTask(id) ? 0 : 1;

    } else {
        cerr << "[ERROR] Unknown command: \"" << command << "\"\n\n";
        printUsage(argv[0]);
        return 1;
    }
}

int main(int argc, char* argv[]) {
    setupConsole();

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