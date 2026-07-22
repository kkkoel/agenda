#include "Task.h"

std::string Task::priorityToString(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "High";
        case Priority::MEDIUM: return "Medium";
        case Priority::LOW:    return "Low";
        default: return "Unknown";
    }
}

std::string Task::categoryToString(Category c) {
    switch (c) {
        case Category::STUDY:         return "Study";
        case Category::ENTERTAINMENT: return "Entertainment";
        case Category::LIFE:          return "Life";
        default: return "Unknown";
    }
}

Priority Task::stringToPriority(const std::string& s) {
    if (s == "High" || s == "HIGH" || s == "high") return Priority::HIGH;
    if (s == "Medium" || s == "MEDIUM" || s == "medium") return Priority::MEDIUM;
    if (s == "Low" || s == "LOW" || s == "low") return Priority::LOW;
    return Priority::MEDIUM;
}

Category Task::stringToCategory(const std::string& s) {
    if (s == "Study" || s == "STUDY" || s == "study") return Category::STUDY;
    if (s == "Entertainment" || s == "ENTERTAINMENT" || s == "entertainment") return Category::ENTERTAINMENT;
    if (s == "Life" || s == "LIFE" || s == "life") return Category::LIFE;
    return Category::LIFE;
}