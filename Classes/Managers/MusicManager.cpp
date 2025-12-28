/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MusicManager.cpp
 * File Function: 音乐管理器实现（兼容层）
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "MusicManager.h"

#include "Audio/AudioManager.h"
#include "cocos2d.h"

using namespace cocos2d;

MusicManager& MusicManager::getInstance() {
  static MusicManager instance;
  return instance;
}

void MusicManager::initialize() {
  // 委托给新的 AudioManager
  AudioManager::GetInstance().Initialize();
}

MusicId MusicManager::ConvertToMusicId(MusicType type) {
  switch (type) {
    case MusicType::BATTLE_GOING:
      return MusicId::kBattleGoing;
    case MusicType::BATTLE_LOSE:
      return MusicId::kBattleLose;
    case MusicType::BATTLE_PREPARING:
      return MusicId::kBattlePreparing;
    case MusicType::BATTLE_WIN:
      return MusicId::kBattleWin;
    default:
      return MusicId::kBattlePreparing;
  }
}

void MusicManager::playMusic(MusicType type, bool loop) {
  current_music_type_ = type;
  MusicId music_id = ConvertToMusicId(type);
  AudioManager::GetInstance().PlayMusic(music_id, loop);
}

void MusicManager::stopMusic() {
  AudioManager::GetInstance().StopMusic();
}

void MusicManager::pauseMusic() {
  AudioManager::GetInstance().PauseMusic();
}

void MusicManager::resumeMusic() {
  AudioManager::GetInstance().ResumeMusic();
}

void MusicManager::setVolume(float volume) {
  AudioManager::GetInstance().SetMusicVolume(volume);
}

float MusicManager::getVolume() const {
  return AudioManager::GetInstance().GetMusicVolume();
}

void MusicManager::setMuted(bool muted) {
  is_muted_ = muted;
  AudioManager::GetInstance().SetMuted(muted);
}

bool MusicManager::isMuted() const {
  return is_muted_;
}

MusicType MusicManager::getCurrentMusicType() const {
  return current_music_type_;
}

bool MusicManager::isPlaying() const {
  return AudioManager::GetInstance().IsMusicPlaying();
}
