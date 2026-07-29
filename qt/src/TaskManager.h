#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Task.h"
#include <vector>
#include <string>
#include <ctime>
#include <mutex>
#include <set>
#include <functional>

class TaskManager {
public:
    explicit TaskManager(const std::string& username);

    bool addTask(const std::string& name, time_t startTime, const std::string& priority, const std::string& category, time_t remindTime);
    bool deleteTask(int id);
    void showTasksForDay(time_t date) const;
    void showAllTasks() const;
    bool loadFromFile();
    bool saveToFile() const;

    std::vector<Task> getTasksForDay(time_t date) const;
    std::vector<Task> getAllTasks() const;

    void checkReminders();

    //设置提醒回调函数
    void setReminderCallback(std::function<void(const Task&)> callback);

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

    //回调函数
    std::function<void(const Task&)> m_reminderCallback;
};

#endif