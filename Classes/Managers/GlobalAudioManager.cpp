/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GlobalAudioManager.cpp
 * File Function: 全局音频管理器实现（兼容层）
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "GlobalAudioManager.h"

#include "Audio/AudioManager.h"

USING_NS_CC;

GlobalAudioManager* GlobalAudioManager::instance_ = nullptr;

GlobalAudioManager& GlobalAudioManager::getInstance() {
  if (!instance_) {
    instance_ = new (std::nothrow) GlobalAudioManager();
  }
  return *instance_;
}

GlobalAudioManager::GlobalAudioManager() {
  // 确保新的 AudioManager 已初始化
  AudioManager::GetInstance().Initialize();
}

// ==================== 音乐管理（委托给 AudioManager）====================

int GlobalAudioManager::playMusic(const std::string& filename) {
  return AudioManager::GetInstance().PlayMusicByPath(filename, true);
}

void GlobalAudioManager::stopMusic() {
  AudioManager::GetInstance().StopMusic();
}

void GlobalAudioManager::setMusicVolume(float volume) {
  AudioManager::GetInstance().SetMusicVolume(volume);
}

float GlobalAudioManager::getMusicVolume() const {
  return AudioManager::GetInstance().GetMusicVolume();
}

// ==================== 音效管理（委托给 AudioManager）====================

int GlobalAudioManager::playEffect(const std::string& filename) {
  return AudioManager::GetInstance().PlayEffectByPath(filename);
}

void GlobalAudioManager::setEffectVolume(float volume) {
  AudioManager::GetInstance().SetEffectVolume(volume);
}

float GlobalAudioManager::getEffectVolume() const {
  return AudioManager::GetInstance().GetEffectVolume();
}

// ==================== 设置持久化（委托给 AudioManager）====================

void GlobalAudioManager::loadSettings() {
  AudioManager::GetInstance().LoadSettings();
}

void GlobalAudioManager::saveSettings() {
  AudioManager::GetInstance().SaveSettings();
}
