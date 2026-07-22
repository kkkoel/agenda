#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Task.h"
#include <vector>
#include <string>

class TaskManager {
public:
    // 构造函数：传入当前登录的用户名
    TaskManager(const std::string& username);

    // 添加任务，成功返回 true
    bool addTask(const std::string& name, 
                 std::time_t startTime, 
                 const std::string& priority, 
                 const std::string& category, 
                 std::time_t remindTime);

    // 按 ID 删除任务
    bool deleteTask(int id);

    // 显示某一天的所有任务（按时间排序）
    void showTasksForDay(std::time_t date) const;

    // 从文件加载任务
    bool loadFromFile();

    // 保存任务到文件
    bool saveToFile() const;

private:
    std::string m_username;          // 当前用户名
    std::vector<Task> m_tasks;       // 任务列表
    int m_nextId;                    // 下一个可用的 ID

    // 生成唯一 ID
    int generateId();

    // 检查任务名称 + 开始时间是否唯一
    bool isUnique(const std::string& name, std::time_t startTime) const;

    // 获取当前用户的任务文件名
    std::string getFilename() const;
};

#endif