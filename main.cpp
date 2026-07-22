#include "account_system.h"
#include <iostream>

int main() {
    std::string username, password;

    std::cout << "=== REGISTER ===\n";
    std::cout << "Username: ";
    std::cin >> username;
    std::cout << "Password: ";
    std::cin >> password;

    if (registerUser(username, password)) {
        std::cout << "Registration successful!\n";
    } else {
        std::cout << "Registration failed.\n";
    }

    std::cout << "\n=== LOGIN ===\n";
    std::cout << "Username: ";
    std::cin >> username;
    std::cout << "Password: ";
    std::cin >> password;

    if (loginUser(username, password)) {
        std::cout << "Login successful! Welcome " << username << "!\n";
    } else {
        std::cout << "Login failed.\n";
    }

    return 0;
}