/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MusicManager.h
 * File Function: 音乐管理器头文件（兼容层）
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>

#include "Audio/AudioTypes.h"
#include "audio/include/AudioEngine.h"

/**
 * @enum MusicType
 * @brief 音乐类型枚举（兼容层）
 * @deprecated 请使用 MusicId 代替
 */
enum class MusicType {
  BATTLE_GOING,      ///< 战斗进行中
  BATTLE_LOSE,       ///< 战斗失败
  BATTLE_PREPARING,  ///< 战斗准备
  BATTLE_WIN         ///< 战斗胜利
};

/**
 * @class MusicManager
 * @brief 音乐管理器（兼容层）
 *
 * @deprecated 此类已废弃，请使用 AudioManager 代替。
 *             保留此类仅为兼容旧代码，内部实现已委托给 AudioManager。
 *
 * 迁移指南：
 * - MusicManager::getInstance() -> AudioManager::GetInstance()
 * - MusicType::BATTLE_GOING -> MusicId::kBattleGoing
 * - playMusic(MusicType) -> AudioManager::GetInstance().PlayMusic(MusicId)
 */
class [[deprecated("使用 AudioManager 代替")]] MusicManager {
 public:
  /**
   * @brief 获取单例实例
   * @return MusicManager& 单例引用
   * @deprecated 使用 AudioManager::GetInstance() 代替
   */
  static MusicManager& getInstance();

  /**
   * @brief 初始化音乐管理器
   * @deprecated 使用 AudioManager::GetInstance().Initialize() 代替
   */
  void initialize();

  /**
   * @brief 播放指定类型的背景音乐
   * @param type 音乐类型
   * @param loop 是否循环
   * @deprecated 使用 AudioManager::GetInstance().PlayMusic(MusicId) 代替
   */
  void playMusic(MusicType type, bool loop = true);

  /**
   * @brief 停止当前播放的音乐
   * @deprecated 使用 AudioManager::GetInstance().StopMusic() 代替
   */
  void stopMusic();

  /**
   * @brief 暂停音乐
   * @deprecated 使用 AudioManager::GetInstance().PauseMusic() 代替
   */
  void pauseMusic();

  /**
   * @brief 恢复音乐
   * @deprecated 使用 AudioManager::GetInstance().ResumeMusic() 代替
   */
  void resumeMusic();

  /**
   * @brief 设置音量
   * @param volume 音量值 (0.0f - 1.0f)
   * @deprecated 使用 AudioManager::GetInstance().SetMusicVolume() 代替
   */
  void setVolume(float volume);

  /**
   * @brief 获取当前音量
   * @deprecated 使用 AudioManager::GetInstance().GetMusicVolume() 代替
   */
  float getVolume() const;

  /**
   * @brief 设置是否静音
   * @param muted 是否静音
   * @deprecated 使用 AudioManager::GetInstance().SetMuted() 代替
   */
  void setMuted(bool muted);

  /**
   * @brief 获取是否静音
   * @deprecated 使用 AudioManager::GetInstance().IsMuted() 代替
   */
  bool isMuted() const;

  /**
   * @brief 获取当前播放的音乐类型
   * @deprecated 使用 AudioManager::GetInstance().GetCurrentMusicId() 代替
   */
  MusicType getCurrentMusicType() const;

  /**
   * @brief 是否正在播放
   * @deprecated 使用 AudioManager::GetInstance().IsMusicPlaying() 代替
   */
  bool isPlaying() const;

 private:
  MusicManager() = default;
  MusicManager(const MusicManager&) = delete;
  MusicManager& operator=(const MusicManager&) = delete;

  /**
   * @brief 将旧的 MusicType 转换为新的 MusicId
   */
  static MusicId ConvertToMusicId(MusicType type);

  /// 当前音乐类型（用于 getCurrentMusicType 兼容）
  MusicType current_music_type_ = MusicType::BATTLE_PREPARING;

  /// 是否静音（用于兼容）
  bool is_muted_ = false;
};
