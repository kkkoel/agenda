#include "CommandHandler.h"
#include "account_system.h"
#include "ReminderManager.h"
#include "VoiceManager.h"
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cctype>
#include <cstdlib>

using namespace std;

extern atomic<bool> g_running;

time_t parseTime(const string& rawTimeStr);
string getCurrentTimeStr();
void showInteractiveHelp();
bool doAddTask(TaskManager& manager, const string& name, const string& timeStr,
               const string& priority, const string& category);
void setupConsole();
void printBanner(const string& subtitle);
void printUsage(const char* progName);

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
    string input;

    cout << "  Edit name (current: " << taskName << "): ";
    getline(cin, input);
    if (!input.empty()) taskName = input;

    cout << "  Edit date (current: " << dateStr << "): ";
    getline(cin, input);
    if (!input.empty()) dateStr = input;

    cout << "  Edit time (current: " << timeStr << "): ";
    getline(cin, input);
    if (!input.empty()) timeStr = input;

    cout << "  Edit priority (current: " << priority << "): ";
    getline(cin, input);
    if (!input.empty()) priority = input;

    cout << "  Edit category (current: " << category << "): ";
    getline(cin, input);
    if (!input.empty()) category = input;

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
