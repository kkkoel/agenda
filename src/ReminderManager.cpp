#include "ReminderManager.h"
#include "TaskManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>

using namespace std;

void playReminderSound() {
#ifdef _WIN32
    int result = system("start alert.wav");
#elif __linux__
    int result = system("aplay alert.wav 2>/dev/null &");
#else
    int result = 0;
#endif
    (void)result; // 播放提示音失败不影响主流程，故意忽略返回值
}

void reminderThreadFunc(TaskManager* manager) {
    while (g_running) {
        if (manager != nullptr) {
            manager->checkReminders();
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
}