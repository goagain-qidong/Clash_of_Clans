/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AudioManager.cpp
 * File Function: 统一音频管理器实现
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#include "AudioManager.h"

#include <algorithm>

USING_NS_CC;

AudioManager& AudioManager::GetInstance() {
  static AudioManager instance;
  return instance;
}

AudioManager::AudioManager() {
  // 构造函数中不执行初始化，等待显式调用 Initialize()
}

void AudioManager::Initialize() {
  if (is_initialized_) {
    return;
  }

  LoadSettings();
  is_initialized_ = true;

  CCLOG("🎵 AudioManager 初始化完成");
}

void AudioManager::Cleanup() {
  StopMusic();
  StopAllEffects();
  cocos2d::AudioEngine::uncacheAll();

  is_initialized_ = false;
  CCLOG("🎵 AudioManager 资源已清理");
}

// ==================== 背景音乐管理 ====================

int AudioManager::PlayMusic(MusicId music_id, bool loop) {
  // 如果正在播放相同的音乐，不做处理
  if (current_music_audio_id_ != kInvalidAudioId &&
      current_music_id_ == music_id) {
    return current_music_audio_id_;
  }

  // 获取音乐路径
  std::string path = AudioConfig::GetInstance().GetMusicPath(music_id);
  if (path.empty()) {
    CCLOG("⚠️ 未找到音乐路径: MusicId=%d", static_cast<int>(music_id));
    return kInvalidAudioId;
  }

  // 停止当前音乐
  StopMusic();

  // 播放新音乐
  float volume = CalculateActualVolume(music_volume_);
  current_music_audio_id_ = cocos2d::AudioEngine::play2d(path, loop, volume);
  current_music_id_ = music_id;

  if (current_music_audio_id_ != kInvalidAudioId) {
    CCLOG("🎵 播放音乐: %s (ID: %d, 音量: %.2f)", path.c_str(),
          current_music_audio_id_, volume);
  }

  return current_music_audio_id_;
}

int AudioManager::PlayMusicByPath(const std::string& file_path, bool loop) {
  // 停止当前音乐
  StopMusic();

  // 播放新音乐
  float volume = CalculateActualVolume(music_volume_);
  current_music_audio_id_ =
      cocos2d::AudioEngine::play2d(file_path, loop, volume);
  current_music_id_ = MusicId::kNone;  // 通过路径播放时，无法确定类型

  if (current_music_audio_id_ != kInvalidAudioId) {
    CCLOG("🎵 播放音乐(路径): %s (ID: %d, 音量: %.2f)", file_path.c_str(),
          current_music_audio_id_, volume);
  }

  return current_music_audio_id_;
}

void AudioManager::StopMusic() {
  if (current_music_audio_id_ != kInvalidAudioId) {
    cocos2d::AudioEngine::stop(current_music_audio_id_);
    CCLOG("🎵 停止音乐 (ID: %d)", current_music_audio_id_);
    current_music_audio_id_ = kInvalidAudioId;
    current_music_id_ = MusicId::kNone;
  }
}

void AudioManager::PauseMusic() {
  if (current_music_audio_id_ != kInvalidAudioId) {
    cocos2d::AudioEngine::pause(current_music_audio_id_);
    CCLOG("🎵 暂停音乐");
  }
}

void AudioManager::ResumeMusic() {
  if (current_music_audio_id_ != kInvalidAudioId) {
    cocos2d::AudioEngine::resume(current_music_audio_id_);
    CCLOG("🎵 恢复音乐");
  }
}

bool AudioManager::IsMusicPlaying() const {
  if (current_music_audio_id_ == kInvalidAudioId) {
    return false;
  }
  return cocos2d::AudioEngine::getState(current_music_audio_id_) ==
         cocos2d::AudioEngine::AudioState::PLAYING;
}

// ==================== 音效管理 ====================

int AudioManager::PlayEffect(SoundEffectId effect_id) {
  // 获取音效路径（可能随机选择变体）
  std::string path = AudioConfig::GetInstance().GetEffectPath(effect_id);
  if (path.empty()) {
    CCLOG("⚠️ 未找到音效路径: SoundEffectId=%d", static_cast<int>(effect_id));
    return kInvalidAudioId;
  }

  return PlayEffectByPath(path);
}

int AudioManager::PlayEffectByPath(const std::string& file_path) {
  float volume = CalculateActualVolume(effect_volume_);
  int audio_id = cocos2d::AudioEngine::play2d(file_path, false, volume);

  if (audio_id != kInvalidAudioId) {
    playing_effect_ids_.insert(audio_id);

    // 设置完成回调，播放完后从集合中移除
    cocos2d::AudioEngine::setFinishCallback(
        audio_id, [this, audio_id](int id, const std::string& file) {
          playing_effect_ids_.erase(audio_id);
        });

    CCLOG("🔔 播放音效: %s (ID: %d, 音量: %.2f)", file_path.c_str(), audio_id,
          volume);
  }

  return audio_id;
}

void AudioManager::StopEffect(int audio_id) {
  if (audio_id != kInvalidAudioId) {
    cocos2d::AudioEngine::stop(audio_id);
    playing_effect_ids_.erase(audio_id);
  }
}

void AudioManager::StopAllEffects() {
  for (int audio_id : playing_effect_ids_) {
    cocos2d::AudioEngine::stop(audio_id);
  }
  playing_effect_ids_.clear();
  CCLOG("🔔 停止所有音效");
}

// ==================== 音量控制 ====================

void AudioManager::SetMusicVolume(float volume) {
  music_volume_ = ClampVolume(volume);

  // 如果有音乐正在播放，立即更新音量
  if (current_music_audio_id_ != kInvalidAudioId) {
    float actual_volume = CalculateActualVolume(music_volume_);
    cocos2d::AudioEngine::setVolume(current_music_audio_id_, actual_volume);
  }

  SaveSettings();
  CCLOG("🔊 音乐音量设置为: %.2f", music_volume_);
}

void AudioManager::SetEffectVolume(float volume) {
  effect_volume_ = ClampVolume(volume);
  UpdateEffectVolumes();
  SaveSettings();
  CCLOG("🔊 音效音量设置为: %.2f", effect_volume_);
}

void AudioManager::SetMasterVolume(float volume) {
  master_volume_ = ClampVolume(volume);

  // 更新所有音频音量
  if (current_music_audio_id_ != kInvalidAudioId) {
    float actual_volume = CalculateActualVolume(music_volume_);
    cocos2d::AudioEngine::setVolume(current_music_audio_id_, actual_volume);
  }
  UpdateEffectVolumes();

  SaveSettings();
  CCLOG("🔊 主音量设置为: %.2f", master_volume_);
}

void AudioManager::SetMuted(bool muted) {
  is_muted_ = muted;

  // 更新所有音频音量
  if (current_music_audio_id_ != kInvalidAudioId) {
    float actual_volume = CalculateActualVolume(music_volume_);
    cocos2d::AudioEngine::setVolume(current_music_audio_id_, actual_volume);
  }
  UpdateEffectVolumes();

  SaveSettings();
  CCLOG("🔇 静音状态: %s", is_muted_ ? "开启" : "关闭");
}

float AudioManager::CalculateActualVolume(float base_volume) const {
  if (is_muted_) {
    return 0.0f;
  }
  return base_volume * master_volume_;
}

void AudioManager::UpdateEffectVolumes() {
  float actual_volume = CalculateActualVolume(effect_volume_);
  for (int audio_id : playing_effect_ids_) {
    if (cocos2d::AudioEngine::getState(audio_id) ==
        cocos2d::AudioEngine::AudioState::PLAYING) {
      cocos2d::AudioEngine::setVolume(audio_id, actual_volume);
    }
  }
}

float AudioManager::ClampVolume(float volume) {
  return std::max(0.0f, std::min(1.0f, volume));
}

// ==================== 预加载 ====================

void AudioManager::PreloadAllEffects() {
  auto paths = AudioConfig::GetInstance().GetAllEffectPaths();
  for (const auto& path : paths) {
    cocos2d::AudioEngine::preload(path);
  }
  CCLOG("🎵 预加载所有音效完成，共 %zu 个文件", paths.size());
}

void AudioManager::PreloadEffect(SoundEffectId effect_id) {
  auto paths = AudioConfig::GetInstance().GetEffectPaths(effect_id);
  for (const auto& path : paths) {
    cocos2d::AudioEngine::preload(path);
  }
}

void AudioManager::PreloadMusic(MusicId music_id) {
  std::string path = AudioConfig::GetInstance().GetMusicPath(music_id);
  if (!path.empty()) {
    cocos2d::AudioEngine::preload(path);
  }
}

// ==================== 设置持久化 ====================

void AudioManager::LoadSettings() {
  auto user_default = UserDefault::getInstance();
  music_volume_ = user_default->getFloatForKey(kKeyMusicVolume, 1.0f);
  effect_volume_ = user_default->getFloatForKey(kKeyEffectVolume, 1.0f);
  master_volume_ = user_default->getFloatForKey(kKeyMasterVolume, 1.0f);
  is_muted_ = user_default->getBoolForKey(kKeyMuted, false);

  CCLOG("📂 音频设置已加载: 音乐=%.2f, 音效=%.2f, 主音量=%.2f, 静音=%s",
        music_volume_, effect_volume_, master_volume_,
        is_muted_ ? "是" : "否");
}

void AudioManager::SaveSettings() {
  auto user_default = UserDefault::getInstance();
  user_default->setFloatForKey(kKeyMusicVolume, music_volume_);
  user_default->setFloatForKey(kKeyEffectVolume, effect_volume_);
  user_default->setFloatForKey(kKeyMasterVolume, master_volume_);
  user_default->setBoolForKey(kKeyMuted, is_muted_);
  user_default->flush();
}
