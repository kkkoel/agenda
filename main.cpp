#include "account_system.h"
#include "TaskManager.h"
#include <iostream>
#include <ctime>
#include <windows.h>

int main() {
    // 强制设置控制台为 UTF-8 模式
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::string username, password;

    std::cout << "=== LOGIN ===\n";
    std::cout << "Username: ";
    std::cin >> username;
    std::cout << "Password: ";
    std::cin >> password;

    if (!loginUser(username, password)) {
        std::cout << "Login failed. Exiting...\n";
        return 1;
    }
    std::cout << "Login successful! Welcome " << username << "!\n\n";

    TaskManager manager(username);

    std::time_t now = std::time(nullptr);

    manager.addTask("HW", now + 3600, "High", "Study", now + 3000);
    manager.addTask("MV", now + 7200, "Low", "Entertainment", now + 7000);

    std::cout << "\n=== Today's Tasks ===\n";
    manager.showTasksForDay(now);

    return 0;
}