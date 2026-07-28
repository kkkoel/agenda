#include "ReminderManager.h"
#include "TaskManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>

using namespace std;

void playReminderSound() {
#ifdef _WIN32
    system("start ..\\alert.wav");
#elif __linux__
    system("aplay ../alert.wav 2>/dev/null &");
#endif
}

void reminderThreadFunc(TaskManager* manager) {
    while (g_running) {
        if (manager != nullptr) {
            manager->checkReminders();
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
}
