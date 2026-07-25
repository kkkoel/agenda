#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

#include <string>

// 检查语音识别所需的外部依赖(arecord、whisper-cli、模型文件)是否都就绪。
// 建议在开始录音流程前先调用一次，未就绪时会打印清晰的诊断信息。
bool voiceDependenciesReady();

bool recordAudio(const std::string& filename, int seconds);
std::string recognizeSpeech(const std::string& filename);
std::string parseDateFromText(const std::string& text);
std::string parseTimeFromText(const std::string& text);
void parsePriorityAndCategory(const std::string& text, std::string& priority, std::string& category);

#endif