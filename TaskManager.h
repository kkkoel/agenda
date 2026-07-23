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

    // 供后台提醒线程周期性调用，内部自带加锁，线程安全
    void checkReminders();

private:
    std::string m_username;
    std::vector<Task> m_tasks;
    int m_nextId;
    std::set<int> m_remindedIds;      // 已经提醒过的任务id，避免重复刷屏
    mutable std::mutex m_mutex;       // 保护 m_tasks / m_nextId / m_remindedIds

    int generateId();
    bool isUnique(const std::string& name, time_t startTime) const; // 调用者必须已持有锁
    std::string getFilename() const;

    // 内部不加锁版本，供已经持有锁的公开接口调用，避免重复加锁死锁
    bool saveToFileUnlocked() const;
};

#endif