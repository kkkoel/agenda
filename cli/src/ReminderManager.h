#ifndef REMINDERMANAGER_H
#define REMINDERMANAGER_H

#include <atomic>
#include <thread>

extern std::atomic<bool> g_running;

class TaskManager;

void playReminderSound();
void reminderThreadFunc(TaskManager* manager);

#endif