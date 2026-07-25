#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Task.h"
#include <vector>
#include <string>
#include <ctime>
#include <mutex>
#include <set>

class TaskManager {
public:
    explicit TaskManager(const std::string& username);

    bool addTask(const std::string& name, time_t startTime, const std::string& priority, const std::string& category, time_t remindTime);
    bool deleteTask(int id);
    void showTasksForDay(time_t date) const;
    void showAllTasks() const;
    bool loadFromFile();
    bool saveToFile() const;

    void checkReminders();

private:
    std::string m_username;
    std::vector<Task> m_tasks;
    int m_nextId;
    std::set<int> m_remindedIds;
    mutable std::mutex m_mutex;

    int generateId();
    bool isUnique(const std::string& name, time_t startTime) const;
    std::string getFilename() const;
    bool saveToFileUnlocked() const;
    void printTaskTable(std::vector<Task> tasks, const std::string& title) const;
};

#endif