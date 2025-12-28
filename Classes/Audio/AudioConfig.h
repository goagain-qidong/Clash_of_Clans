/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AudioConfig.h
 * File Function: 音频配置管理头文件
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#ifndef AUDIO_CONFIG_H_
#define AUDIO_CONFIG_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "AudioTypes.h"

/**
 * @class AudioConfig
 * @brief 音频配置管理器（单例）
 *
 * 统一管理所有音频文件的路径配置，提供类型安全的音频路径查询。
 * 支持同一音效类型对应多个音频文件（随机播放）。
 */
class AudioConfig {
 public:
  /**
   * @brief 获取单例实例
   * @return AudioConfig& 单例引用
   */
  static AudioConfig& GetInstance();

  // 禁止拷贝和赋值
  AudioConfig(const AudioConfig&) = delete;
  AudioConfig& operator=(const AudioConfig&) = delete;

  /**
   * @brief 获取背景音乐路径
   * @param music_id 音乐类型
   * @return 音乐文件路径，如果不存在则返回空字符串
   */
  std::string GetMusicPath(MusicId music_id) const;

  /**
   * @brief 获取音效路径
   * @param effect_id 音效类型
   * @return 音效文件路径，如果存在多个则随机返回一个
   */
  std::string GetEffectPath(SoundEffectId effect_id) const;

  /**
   * @brief 获取某类音效的所有路径
   * @param effect_id 音效类型
   * @return 该类型对应的所有音效文件路径
   */
  std::vector<std::string> GetEffectPaths(SoundEffectId effect_id) const;

  /**
   * @brief 获取所有需要预加载的音效路径
   * @return 所有音效文件路径列表
   */
  std::vector<std::string> GetAllEffectPaths() const;

  /**
   * @brief 获取所有需要预加载的音乐路径
   * @return 所有音乐文件路径列表
   */
  std::vector<std::string> GetAllMusicPaths() const;

 private:
  AudioConfig();
  ~AudioConfig() = default;

  /**
   * @brief 初始化音乐路径映射表
   */
  void InitMusicPaths();

  /**
   * @brief 初始化音效路径映射表
   */
  void InitEffectPaths();

  /**
   * @brief 音频资源根目录
   */
  static constexpr const char* kAudioBasePath = "audio/";
  static constexpr const char* kBackgroundPath = "audio/background/";
  static constexpr const char* kSoundEffectsPath = "audio/sound_effects/";

  /// 音乐ID到路径的映射
  std::unordered_map<MusicId, std::string> music_paths_;

  /// 音效ID到路径列表的映射（支持多个变体随机播放）
  std::unordered_map<SoundEffectId, std::vector<std::string>> effect_paths_;
};

#endif  // AUDIO_CONFIG_H_
