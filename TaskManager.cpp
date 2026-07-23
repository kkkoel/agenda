#include "TaskManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <cstdlib>

using namespace std;

static const int W_ID       = 5;
static const int W_NAME     = 24;
static const int W_START    = 18;
static const int W_PRIORITY = 9;
static const int W_CATEGORY = 14;
static const int W_REMIND   = 18;

static string timeToStr(time_t t) {
    if (t <= 0) return "N/A";
    char buf[64];
    struct tm* local = localtime(&t);
    if (local == nullptr) return "Error";
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", local);
    return string(buf);
}

static string truncateField(const string& s, size_t width) {
    if (s.size() <= width) return s;
    if (width <= 2) return s.substr(0, width);
    return s.substr(0, width - 2) + "..";
}

static void printSeparatorLine() {
    int total = W_ID + W_NAME + W_START + W_PRIORITY + W_CATEGORY + W_REMIND + 13;
    cout << "+" << string(total - 2, '-') << "+\n";
}

static void playReminderSound() {
#ifdef _WIN32
    system("start alert.wav");
#elif __linux__
    system("aplay alert.wav 2>/dev/null &");
#elif __APPLE__
    system("afplay alert.wav 2>/dev/null &");
#endif
}

TaskManager::TaskManager(const string& username) : m_username(username), m_nextId(1) {
    loadFromFile();
}

string TaskManager::getFilename() const {
    return m_username + "_tasks.txt";
}

int TaskManager::generateId() {
    return m_nextId++;
}

bool TaskManager::isUnique(const string& name, time_t startTime) const {
    for (const auto& task : m_tasks) {
        if (task.name == name && task.startTime == startTime) {
            return false;
        }
    }
    return true;
}

bool TaskManager::addTask(const string& name, time_t startTime, const string& priority, const string& category, time_t remindTime) {
    lock_guard<mutex> lock(m_mutex);

    if (!isUnique(name, startTime)) {
        cerr << "[ERROR] A task with the same name and start time already exists.\n";
        cerr << "        Task name + start time must be unique.\n";
        return false;
    }

    Task task;
    task.id = generateId();
    task.name = name;
    task.startTime = startTime;
    task.priority = Task::stringToPriority(priority);
    task.category = Task::stringToCategory(category);
    task.remindTime = remindTime;

    m_tasks.push_back(task);
    saveToFileUnlocked();

    cout << "\n[OK] Task #" << task.id << " \"" << task.name << "\" created.\n";
    cout << "     Starts: " << timeToStr(task.startTime)
         << "  |  Priority: " << Task::priorityToString(task.priority)
         << "  |  Category: " << Task::categoryToString(task.category)
         << "  |  Reminder: " << timeToStr(task.remindTime) << "\n";

    return true;
}

bool TaskManager::deleteTask(int id) {
    lock_guard<mutex> lock(m_mutex);

    auto it = find_if(m_tasks.begin(), m_tasks.end(), [id](const Task& t) { return t.id == id; });
    if (it == m_tasks.end()) {
        cerr << "[ERROR] Task with ID " << id << " not found.\n";
        return false;
    }
    string name = it->name;
    m_tasks.erase(it);
    m_remindedIds.erase(id);
    saveToFileUnlocked();

    cout << "[OK] Task #" << id << " \"" << name << "\" deleted.\n";
    return true;
}

void TaskManager::printTaskTable(vector<Task> tasks, const string& title) const {
    if (tasks.empty()) {
        cout << "\n" << title << ": no tasks found.\n";
        return;
    }

    sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.startTime < b.startTime;
    });

    cout << "\n" << title << "  (" << tasks.size() << " task"
         << (tasks.size() > 1 ? "s" : "") << ")\n";

    printSeparatorLine();
    cout << "| " << left
         << setw(W_ID) << "ID" << "| "
         << setw(W_NAME) << "Name" << "| "
         << setw(W_START) << "Start" << "| "
         << setw(W_PRIORITY) << "Priority" << "| "
         << setw(W_CATEGORY) << "Category" << "| "
         << setw(W_REMIND) << "Reminder" << "|\n";
    printSeparatorLine();

    for (const auto& task : tasks) {
        cout << "| " << left
             << setw(W_ID) << task.id << "| "
             << setw(W_NAME) << truncateField(task.name, W_NAME) << "| "
             << setw(W_START) << timeToStr(task.startTime) << "| "
             << setw(W_PRIORITY) << Task::priorityToString(task.priority) << "| "
             << setw(W_CATEGORY) << Task::categoryToString(task.category) << "| "
             << setw(W_REMIND) << timeToStr(task.remindTime) << "|\n";
    }
    printSeparatorLine();
}

void TaskManager::showTasksForDay(time_t date) const {
    lock_guard<mutex> lock(m_mutex);

    tm* local = localtime(&date);
    local->tm_hour = 0;
    local->tm_min = 0;
    local->tm_sec = 0;
    time_t dayStart = mktime(local);
    time_t dayEnd = dayStart + 24 * 60 * 60;

    vector<Task> dayTasks;
    for (const auto& task : m_tasks) {
        if (task.startTime >= dayStart && task.startTime < dayEnd) {
            dayTasks.push_back(task);
        }
    }

    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", local);
    printTaskTable(dayTasks, string("Tasks on ") + dateBuf);
}

void TaskManager::showAllTasks() const {
    lock_guard<mutex> lock(m_mutex);
    printTaskTable(m_tasks, "All Tasks (" + m_username + ")");
}

bool TaskManager::loadFromFile() {
    lock_guard<mutex> lock(m_mutex);

    string filename = getFilename();
    ifstream fin(filename);
    if (!fin.is_open()) return true;

    m_tasks.clear();
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        Task task;
        string priorityStr, categoryStr, startTimeStr, remindTimeStr;

        getline(ss, startTimeStr, '|');
        getline(ss, priorityStr, '|');
        getline(ss, categoryStr, '|');
        getline(ss, remindTimeStr, '|');
        getline(ss, line);

        task.startTime = stoll(startTimeStr);
        task.priority = Task::stringToPriority(priorityStr);
        task.category = Task::stringToCategory(categoryStr);
        task.remindTime = stoll(remindTimeStr);
        task.name = line;

        getline(fin, line);
        task.id = stoi(line);

        m_tasks.push_back(task);
        if (task.id >= m_nextId) m_nextId = task.id + 1;
    }
    return true;
}

bool TaskManager::saveToFile() const {
    lock_guard<mutex> lock(m_mutex);
    return saveToFileUnlocked();
}

bool TaskManager::saveToFileUnlocked() const {
    string filename = getFilename();
    ofstream fout(filename);
    if (!fout.is_open()) {
        cerr << "[ERROR] Cannot open " << filename << " for writing.\n";
        return false;
    }

    for (const auto& task : m_tasks) {
        fout << task.startTime << "|" << Task::priorityToString(task.priority) << "|"
             << Task::categoryToString(task.category) << "|" << task.remindTime << "|" << task.name << "\n";
        fout << task.id << "\n";
    }
    return true;
}

void TaskManager::checkReminders() {
    lock_guard<mutex> lock(m_mutex);

    time_t now = time(nullptr);
    for (const auto& task : m_tasks) {
        bool alreadyReminded = (m_remindedIds.find(task.id) != m_remindedIds.end());
        bool timeReached = (task.remindTime > 0 && task.remindTime <= now);
        bool notStartedYet = (task.startTime >= now);

        if (timeReached && notStartedYet && !alreadyReminded) {
            playReminderSound();

            cout << "\n"
                 << "+--------------------------------------------------------+\n"
                 << "|  REMINDER                                              |\n"
                 << "+--------------------------------------------------------+\n"
                 << "  Task:     " << task.name << "\n"
                 << "  Starts:   " << timeToStr(task.startTime) << "\n"
                 << "  Priority: " << Task::priorityToString(task.priority) << "\n"
                 << "  Category: " << Task::categoryToString(task.category) << "\n"
                 << "> " << flush;
            m_remindedIds.insert(task.id);
        }
    }
}