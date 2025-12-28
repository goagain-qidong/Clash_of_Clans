/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GlobalAudioManager.h
 * File Function: 全局音频管理器头文件（兼容层）
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>
#include <vector>

#include "audio/include/AudioEngine.h"
#include "cocos2d.h"

// 前向声明新的音频管理器
class AudioManager;

/**
 * @class GlobalAudioManager
 * @brief 全局音频管理器（兼容层）
 *
 * @deprecated 此类已废弃，请使用 AudioManager 代替。
 *             保留此类仅为兼容旧代码，内部实现已委托给 AudioManager。
 *
 * 迁移指南：
 * - GlobalAudioManager::getInstance() -> AudioManager::GetInstance()
 * - playMusic(path) -> AudioManager::GetInstance().PlayMusic(MusicId)
 * - playEffect(path) -> AudioManager::GetInstance().PlayEffect(SoundEffectId)
 */
class [[deprecated("使用 AudioManager 代替")]] GlobalAudioManager {
 public:
  /**
   * @brief 获取单例实例
   * @return GlobalAudioManager& 单例引用
   * @deprecated 使用 AudioManager::GetInstance() 代替
   */
  static GlobalAudioManager& getInstance();

  /**
   * @brief 播放背景音乐（循环）
   * @param filename 音乐文件路径
   * @return int 音频ID
   * @deprecated 使用 AudioManager::GetInstance().PlayMusic(MusicId) 代替
   */
  int playMusic(const std::string& filename);

  /**
   * @brief 停止当前音乐
   * @deprecated 使用 AudioManager::GetInstance().StopMusic() 代替
   */
  void stopMusic();

  /**
   * @brief 设置音乐音量
   * @param volume 音量（0.0 ~ 1.0）
   * @deprecated 使用 AudioManager::GetInstance().SetMusicVolume() 代替
   */
  void setMusicVolume(float volume);

  /**
   * @brief 获取音乐音量
   * @deprecated 使用 AudioManager::GetInstance().GetMusicVolume() 代替
   */
  float getMusicVolume() const;

  /**
   * @brief 播放音效
   * @param filename 音效文件路径
   * @return int 音效ID
   * @deprecated 使用 AudioManager::GetInstance().PlayEffect(SoundEffectId) 代替
   */
  int playEffect(const std::string& filename);

  /**
   * @brief 设置音效音量
   * @param volume 音量（0.0 ~ 1.0）
   * @deprecated 使用 AudioManager::GetInstance().SetEffectVolume() 代替
   */
  void setEffectVolume(float volume);

  /**
   * @brief 获取音效音量
   * @deprecated 使用 AudioManager::GetInstance().GetEffectVolume() 代替
   */
  float getEffectVolume() const;

  /**
   * @brief 加载设置
   * @deprecated 使用 AudioManager::GetInstance().LoadSettings() 代替
   */
  void loadSettings();

  /**
   * @brief 保存设置
   * @deprecated 使用 AudioManager::GetInstance().SaveSettings() 代替
   */
  void saveSettings();

 private:
  GlobalAudioManager();
  ~GlobalAudioManager() = default;

  static GlobalAudioManager* instance_;  ///< 单例实例
};
