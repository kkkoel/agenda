#ifndef ACCOUNT_SYSTEM_H
#define ACCOUNT_SYSTEM_H

#include <string>

std::string sha256(const std::string& input);
bool registerUser(const std::string& username, const std::string& password);
bool loginUser(const std::string& username, const std::string& password);

#endif