#ifndef TASK_H
#define TASK_H

#include <string>
#include <ctime>

enum class Priority {
    HIGH,
    MEDIUM,
    LOW
};

enum class Category {
    STUDY,
    ENTERTAINMENT,
    LIFE
};

struct Task {
    int id;
    std::string name;
    std::time_t startTime;
    Priority priority;
    Category category;
    std::time_t remindTime;

    static std::string priorityToString(Priority p);
    static std::string categoryToString(Category c);
    static Priority stringToPriority(const std::string& s);
    static Category stringToCategory(const std::string& s);
};

#endif