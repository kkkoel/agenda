#include "TaskManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

TaskManager::TaskManager(const std::string& username)
    : m_username(username), m_nextId(1) {
    loadFromFile();
}

std::string TaskManager::getFilename() const {
    return m_username + "_tasks.txt";
}

int TaskManager::generateId() {
    return m_nextId++;
}

bool TaskManager::isUnique(const std::string& name, std::time_t startTime) const {
    for (const auto& task : m_tasks) {
        if (task.name == name && task.startTime == startTime) {
            return false;
        }
    }
    return true;
}

bool TaskManager::addTask(const std::string& name,
                          std::time_t startTime,
                          const std::string& priority,
                          const std::string& category,
                          std::time_t remindTime) {
    // 检查名称 + 时间是否唯一
    if (!isUnique(name, startTime)) {
        std::cerr << "[ERROR] Task name + start time must be unique!\n";
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
    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
                           [id](const Task& t) { return t.id == id; });
    if (it == m_tasks.end()) {
        std::cerr << "[ERROR] Task with ID " << id << " not found!\n";
        return false;
    }
    m_tasks.erase(it);
    saveToFile();
    return true;
}

void TaskManager::showTasksForDay(std::time_t date) const {
    // 把 date 转换成当天的开始时间（0点）
    std::tm* local = std::localtime(&date);
    local->tm_hour = 0;
    local->tm_min = 0;
    local->tm_sec = 0;
    std::time_t dayStart = std::mktime(local);
    std::time_t dayEnd = dayStart + 24 * 60 * 60;

    // 筛选当天的任务
    std::vector<Task> dayTasks;
    for (const auto& task : m_tasks) {
        if (task.startTime >= dayStart && task.startTime < dayEnd) {
            dayTasks.push_back(task);
        }
    }

    // 按开始时间排序
    std::sort(dayTasks.begin(), dayTasks.end(),
              [](const Task& a, const Task& b) {
                  return a.startTime < b.startTime;
              });

    // 显示
    if (dayTasks.empty()) {
        std::cout << "No tasks for this day.\n";
        return;
    }

    std::cout << std::left
              << std::setw(6) << "ID"
              << std::setw(20) << "Name"
              << std::setw(20) << "Start Time"
              << std::setw(10) << "Priority"
              << std::setw(10) << "Category"
              << std::setw(20) << "Remind Time"
              << "\n";
    std::cout << std::string(86, '-') << "\n";

    for (const auto& task : dayTasks) {
        std::cout << std::left
                  << std::setw(6) << task.id
                  << std::setw(20) << task.name
                  << std::setw(20) << std::ctime(&task.startTime)
                  << std::setw(10) << Task::priorityToString(task.priority)
                  << std::setw(10) << Task::categoryToString(task.category)
                  << std::setw(20) << std::ctime(&task.remindTime)
                  << "\n";
    }
}

bool TaskManager::loadFromFile() {
    std::string filename = getFilename();
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        // 文件不存在，不是错误
        return true;
    }

    m_tasks.clear();
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        Task task;
        std::string priorityStr, categoryStr;
        std::string startTimeStr, remindTimeStr;

        std::getline(ss, startTimeStr, '|');
        std::getline(ss, priorityStr, '|');
        std::getline(ss, categoryStr, '|');
        std::getline(ss, remindTimeStr, '|');
        std::getline(ss, line); // 剩余部分是任务名称

        task.startTime = std::stoll(startTimeStr);
        task.priority = Task::stringToPriority(priorityStr);
        task.category = Task::stringToCategory(categoryStr);
        task.remindTime = std::stoll(remindTimeStr);
        task.name = line;

        // ID 单独处理
        std::getline(fin, line);
        task.id = std::stoi(line);

        m_tasks.push_back(task);
        if (task.id >= m_nextId) {
            m_nextId = task.id + 1;
        }
    }
    return true;
}

bool TaskManager::saveToFile() const {
    std::string filename = getFilename();
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cerr << "[ERROR] Cannot open " << filename << " for writing!\n";
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