/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AudioManager.h
 * File Function: 统一音频管理器头文件
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#ifndef AUDIO_MANAGER_H_
#define AUDIO_MANAGER_H_

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "AudioConfig.h"
#include "AudioTypes.h"
#include "audio/include/AudioEngine.h"
#include "cocos2d.h"

/**
 * @class AudioManager
 * @brief 统一音频管理器（单例）
 *
 * 整合背景音乐和音效的播放、音量控制、预加载等功能。
 * 提供类型安全的API，避免硬编码文件路径。
 *
 * 主要功能：
 * - 背景音乐管理（播放、暂停、恢复、停止）
 * - 音效管理（播放、停止）
 * - 音量控制（分离音乐和音效音量）
 * - 音频预加载
 * - 设置持久化
 */
class AudioManager {
 public:
  /// 无效音频ID常量
  static const int kInvalidAudioId = -1;

  /**
   * @brief 获取单例实例
   * @return AudioManager& 单例引用
   */
  static AudioManager& GetInstance();

  // 禁止拷贝和赋值
  AudioManager(const AudioManager&) = delete;
  AudioManager& operator=(const AudioManager&) = delete;

  // ==================== 初始化 ====================

  /**
   * @brief 初始化音频管理器
   *
   * 加载用户设置，执行必要的预加载。
   * 应在游戏启动时调用一次。
   */
  void Initialize();

  /**
   * @brief 释放所有音频资源
   *
   * 停止所有音频，清理缓存。
   * 应在游戏退出时调用。
   */
  void Cleanup();

  // ==================== 背景音乐管理 ====================

  /**
   * @brief 播放背景音乐
   * @param music_id 音乐类型
   * @param loop 是否循环播放，默认true
   * @return 音频ID，失败返回kInvalidAudioId
   */
  int PlayMusic(MusicId music_id, bool loop = true);

  /**
   * @brief 使用文件路径播放背景音乐（兼容旧代码）
   * @param file_path 音乐文件路径
   * @param loop 是否循环播放
   * @return 音频ID
   * @deprecated 推荐使用 PlayMusic(MusicId) 版本
   */
  int PlayMusicByPath(const std::string& file_path, bool loop = true);

  /** @brief 停止当前背景音乐 */
  void StopMusic();

  /** @brief 暂停背景音乐 */
  void PauseMusic();

  /** @brief 恢复背景音乐 */
  void ResumeMusic();

  /** @brief 获取当前正在播放的音乐类型 */
  MusicId GetCurrentMusicId() const { return current_music_id_; }

  /** @brief 检查是否正在播放音乐 */
  bool IsMusicPlaying() const;

  // ==================== 音效管理 ====================

  /**
   * @brief 播放音效
   * @param effect_id 音效类型
   * @return 音频ID，失败返回kInvalidAudioId
   */
  int PlayEffect(SoundEffectId effect_id);

  /**
   * @brief 使用文件路径播放音效（兼容旧代码）
   * @param file_path 音效文件路径
   * @return 音频ID
   * @deprecated 推荐使用 PlayEffect(SoundEffectId) 版本
   */
  int PlayEffectByPath(const std::string& file_path);

  /**
   * @brief 停止指定音效
   * @param audio_id 音频ID
   */
  void StopEffect(int audio_id);

  /** @brief 停止所有音效 */
  void StopAllEffects();

  // ==================== 音量控制 ====================

  /**
   * @brief 设置音乐音量
   * @param volume 音量值（0.0 ~ 1.0）
   */
  void SetMusicVolume(float volume);

  /** @brief 获取音乐音量 */
  float GetMusicVolume() const { return music_volume_; }

  /**
   * @brief 设置音效音量
   * @param volume 音量值（0.0 ~ 1.0）
   */
  void SetEffectVolume(float volume);

  /** @brief 获取音效音量 */
  float GetEffectVolume() const { return effect_volume_; }

  /**
   * @brief 设置主音量（同时影响音乐和音效）
   * @param volume 音量值（0.0 ~ 1.0）
   */
  void SetMasterVolume(float volume);

  /** @brief 获取主音量 */
  float GetMasterVolume() const { return master_volume_; }

  /**
   * @brief 设置静音状态
   * @param muted 是否静音
   */
  void SetMuted(bool muted);

  /** @brief 获取静音状态 */
  bool IsMuted() const { return is_muted_; }

  // ==================== 预加载 ====================

  /**
   * @brief 预加载所有音效
   *
   * 异步加载所有音效文件到内存，减少首次播放延迟。
   */
  void PreloadAllEffects();

  /**
   * @brief 预加载指定音效
   * @param effect_id 音效类型
   */
  void PreloadEffect(SoundEffectId effect_id);

  /**
   * @brief 预加载指定音乐
   * @param music_id 音乐类型
   */
  void PreloadMusic(MusicId music_id);

  // ==================== 设置持久化 ====================

  /** @brief 加载音频设置 */
  void LoadSettings();

  /** @brief 保存音频设置 */
  void SaveSettings();

 private:
  AudioManager();
  ~AudioManager() = default;

  /**
   * @brief 计算实际播放音量
   * @param base_volume 基础音量
   * @return 考虑主音量和静音状态后的实际音量
   */
  float CalculateActualVolume(float base_volume) const;

  /**
   * @brief 更新所有正在播放的音效音量
   */
  void UpdateEffectVolumes();

  /**
   * @brief 限制音量值在有效范围内
   * @param volume 输入音量
   * @return 限制后的音量（0.0 ~ 1.0）
   */
  static float ClampVolume(float volume);

  // ==================== 成员变量 ====================

  /// 当前播放的音乐ID
  int current_music_audio_id_ = kInvalidAudioId;

  /// 当前音乐类型
  MusicId current_music_id_ = MusicId::kNone;

  /// 正在播放的音效ID集合
  std::unordered_set<int> playing_effect_ids_;

  /// 音乐音量
  float music_volume_ = 1.0f;

  /// 音效音量
  float effect_volume_ = 1.0f;

  /// 主音量
  float master_volume_ = 1.0f;

  /// 静音状态
  bool is_muted_ = false;

  /// 是否已初始化
  bool is_initialized_ = false;

  // ==================== 设置键名常量 ====================

  static constexpr const char* kKeyMusicVolume = "AudioManager_MusicVolume";
  static constexpr const char* kKeyEffectVolume = "AudioManager_EffectVolume";
  static constexpr const char* kKeyMasterVolume = "AudioManager_MasterVolume";
  static constexpr const char* kKeyMuted = "AudioManager_Muted";
};

#endif  // AUDIO_MANAGER_H_
