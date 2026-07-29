#include "Task.h"

using namespace std;

string Task::priorityToString(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "高";
        case Priority::MEDIUM: return "中";
        case Priority::LOW:    return "低";
        default: return "未知";
    }
}

string Task::categoryToString(Category c) {
    switch (c) {
        case Category::STUDY:         return "学习";
        case Category::ENTERTAINMENT: return "娱乐";
        case Category::LIFE:          return "生活";
        default: return "未知";
    }
}

Priority Task::stringToPriority(const string& s) {
    if (s == "高" || s == "High" || s == "HIGH" || s == "high") return Priority::HIGH;
    if (s == "中" || s == "Medium" || s == "MEDIUM" || s == "medium") return Priority::MEDIUM;
    if (s == "低" || s == "Low" || s == "LOW" || s == "low") return Priority::LOW;
    return Priority::MEDIUM;
}

Category Task::stringToCategory(const string& s) {
    if (s == "学习" || s == "Study" || s == "STUDY" || s == "study") return Category::STUDY;
    if (s == "娱乐" || s == "Entertainment" || s == "ENTERTAINMENT" || s == "entertainment") return Category::ENTERTAINMENT;
    if (s == "生活" || s == "Life" || s == "LIFE" || s == "life") return Category::LIFE;
    return Category::LIFE;
}
