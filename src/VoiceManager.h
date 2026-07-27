#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

#include <string>

bool recordAudio(const std::string& filename, int seconds);
std::string recognizeSpeech(const std::string& filename);
std::string parseDateFromText(const std::string& text);
std::string parseTimeFromText(const std::string& text);
void parsePriorityAndCategory(const std::string& text, std::string& priority, std::string& category);

#endif