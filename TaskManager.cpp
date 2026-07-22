#include "TaskManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>

using namespace std;

// 辅助函数：将时间戳转为可读字符串
string timeToStr(time_t t) {
    tm* local = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", local);
    return string(buf);
}

TaskManager::TaskManager(const string& username)
    : m_username(username), m_nextId(1) {
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

bool TaskManager::addTask(const string& name,
                          time_t startTime,
                          const string& priority,
                          const string& category,
                          time_t remindTime) {
    if (!isUnique(name, startTime)) {
        cerr << "[ERROR] Task name + start time must be unique!\n";
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
    saveToFile();
    return true;
}

bool TaskManager::deleteTask(int id) {
    auto it = find_if(m_tasks.begin(), m_tasks.end(),
                      [id](const Task& t) { return t.id == id; });
    if (it == m_tasks.end()) {
        cerr << "[ERROR] Task with ID " << id << " not found!\n";
        return false;
    }
    m_tasks.erase(it);
    saveToFile();
    return true;
}

void TaskManager::showTasksForDay(time_t date) const {
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

    sort(dayTasks.begin(), dayTasks.end(),
         [](const Task& a, const Task& b) {
             return a.startTime < b.startTime;
         });

    if (dayTasks.empty()) {
        cout << "No tasks for this day.\n";
        return;
    }

    cout << left
         << setw(6) << "ID"
         << setw(20) << "Name"
         << setw(20) << "Start Time"
         << setw(10) << "Priority"
         << setw(15) << "Category"
         << setw(20) << "Remind Time"
         << "\n";
    cout << string(91, '-') << "\n";

    for (const auto& task : dayTasks) {
        cout << left
             << setw(6) << task.id
             << setw(20) << task.name
             << setw(20) << timeToStr(task.startTime)
             << setw(10) << Task::priorityToString(task.priority)
             << setw(15) << Task::categoryToString(task.category)
             << setw(20) << timeToStr(task.remindTime)
             << "\n";
    }
}

void TaskManager::showAllTasks() const {
    if (m_tasks.empty()) {
        cout << "No tasks at all.\n";
        return;
    }

    cout << "=== All Tasks ===\n";
    for (const auto& task : m_tasks) {
        cout << "ID: " << task.id
             << ", Name: " << task.name
             << ", Start: " << timeToStr(task.startTime) << "\n";
    }
}

bool TaskManager::loadFromFile() {
    string filename = getFilename();
    ifstream fin(filename);
    if (!fin.is_open()) {
        return true;
    }

    m_tasks.clear();
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        Task task;
        string priorityStr, categoryStr;
        string startTimeStr, remindTimeStr;

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
        if (task.id >= m_nextId) {
            m_nextId = task.id + 1;
        }
    }
    return true;
}

bool TaskManager::saveToFile() const {
    string filename = getFilename();
    ofstream fout(filename);
    if (!fout.is_open()) {
        cerr << "[ERROR] Cannot open " << filename << " for writing!\n";
        return false;
    }

    for (const auto& task : m_tasks) {
        fout << task.startTime << "|"
             << Task::priorityToString(task.priority) << "|"
             << Task::categoryToString(task.category) << "|"
             << task.remindTime << "|"
             << task.name << "\n";
        fout << task.id << "\n";
    }
    return true;
}