#include "TaskManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>


using namespace std;

string timeToStr(time_t t) {
    if (t <= 0) return "Invalid";
    char buf[64];
    struct tm* local = localtime(&t);
    if (local == nullptr) return "Error";
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", local);
    return string(buf);
}

time_t timeToMinute(time_t t) {
    return t / 60;
}

TaskManager::TaskManager(const string& username) : m_username(username), m_nextId(1) {
    loadFromFile();
}

string TaskManager::getFilename() const {
    return "/home/code/Desktop/MySchedule/data/" + m_username + "_tasks.txt";
}

int TaskManager::generateId() {
    return m_nextId++;
}

bool TaskManager::isUnique(const string& name, time_t startTime) const {
    time_t newTimeMinutes = timeToMinute(startTime);

    if (m_tasks.empty()) {
        return true;
    }

    for (const auto& task : m_tasks) {
        time_t existingTimeMinutes = timeToMinute(task.startTime);

        if (existingTimeMinutes == newTimeMinutes) {
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


    return true;
}

bool TaskManager::deleteTask(int id) {
    lock_guard<mutex> lock(m_mutex);

    auto it = find_if(m_tasks.begin(), m_tasks.end(), [id](const Task& t) { return t.id == id; });
    if (it == m_tasks.end()) {
        cerr << "[ERROR] Task with ID " << id << " not found!\n";
        return false;
    }

    m_tasks.erase(it);
    m_remindedIds.erase(id);
    saveToFileUnlocked();

    return true;
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

    sort(dayTasks.begin(), dayTasks.end(), [](const Task& a, const Task& b) {
        return a.startTime < b.startTime;
    });

    if (dayTasks.empty()) {
        cout << "No tasks for this day.\n";
        return;
    }

    cout << left << setw(6) << "ID" << setw(20) << "Name" << setw(20) << "Start Time"
         << setw(10) << "Priority" << setw(15) << "Category" << setw(20) << "Remind Time" << "\n";
    cout << string(91, '-') << "\n";

    for (const auto& task : dayTasks) {
        cout << left << setw(6) << task.id << setw(20) << task.name
             << setw(20) << timeToStr(task.startTime)
             << setw(10) << Task::priorityToString(task.priority)
             << setw(15) << Task::categoryToString(task.category)
             << setw(20) << timeToStr(task.remindTime) << "\n";
    }
}

void TaskManager::showAllTasks() const {
    lock_guard<mutex> lock(m_mutex);

    if (m_tasks.empty()) {
        cout << "No tasks at all.\n";
        return;
    }

    cout << "=== All Tasks ===\n";
    for (const auto& task : m_tasks) {
        cout << "ID: " << task.id << ", Name: " << task.name << ", Start: " << timeToStr(task.startTime) << "\n";
    }
}

bool TaskManager::loadFromFile() {
    lock_guard<mutex> lock(m_mutex);

    string filename = getFilename();

    ifstream fin(filename);
    if (!fin.is_open()) {
        return true;
    }

    m_tasks.clear();
    string line;
    int loadedCount = 0;

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
        loadedCount++;
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
        cerr << "[ERROR] Cannot open " << filename << " for writing!\n";
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
    time_t now = time(nullptr);
    
    vector<Task> tasks_copy;
    {
        lock_guard<mutex> lock(m_mutex);
        tasks_copy = m_tasks;
    }
    
    for (const auto& task : tasks_copy) {
        bool alreadyReminded = (m_remindedIds.find(task.id) != m_remindedIds.end());
        bool timeReached = (task.remindTime > 0 && task.remindTime <= now);
        bool notStartedYet = (task.startTime >= now);
        
        if (timeReached && notStartedYet && !alreadyReminded) {
            if (m_reminderCallback) {
                m_reminderCallback(task);
            }
            {
                lock_guard<mutex> lock(m_mutex);
                m_remindedIds.insert(task.id);
            }
        }
    }
}

std::vector<Task> TaskManager::getTasksForDay(time_t date) const {
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

    sort(dayTasks.begin(), dayTasks.end(), [](const Task& a, const Task& b) {
        return a.startTime < b.startTime;
    });

    return dayTasks;
}

std::vector<Task> TaskManager::getAllTasks() const {
    lock_guard<mutex> lock(m_mutex);
    return m_tasks;
}

void TaskManager::setReminderCallback(std::function<void(const Task&)> callback) {
    m_reminderCallback = callback;
}

