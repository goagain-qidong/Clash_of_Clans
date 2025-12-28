/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AudioConfig.cpp
 * File Function: 音频配置管理实现
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#include "AudioConfig.h"

#include <cstdlib>
#include <ctime>

AudioConfig& AudioConfig::GetInstance() {
  static AudioConfig instance;
  return instance;
}

AudioConfig::AudioConfig() {
  // 初始化随机数种子
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  InitMusicPaths();
  InitEffectPaths();
}

void AudioConfig::InitMusicPaths() {
  // 战斗相关音乐
  music_paths_[MusicId::kBattlePreparing] =
      std::string(kBackgroundPath) + "Battle_Preparing.mp3";
  music_paths_[MusicId::kBattleGoing] =
      std::string(kBackgroundPath) + "Battle_Going.mp3";
  music_paths_[MusicId::kBattleWin] =
      std::string(kBackgroundPath) + "Battle_Win.mp3";
  music_paths_[MusicId::kBattleLose] =
      std::string(kBackgroundPath) + "Battle_Lose.mp3";
}

void AudioConfig::InitEffectPaths() {
  const std::string base = kSoundEffectsPath;

  // ==================== UI 音效 ====================
  effect_paths_[SoundEffectId::kUiButtonClick] = {
      base + "ui/button_click.mp3"};
  effect_paths_[SoundEffectId::kUiLoadingEnter] = {
      base + "ui/loading_enter.mp3"};
  effect_paths_[SoundEffectId::kUiSupercellLogo] = {
      base + "ui/supercell_logo.mp3"};

  // ==================== 资源采集音效 ====================
  effect_paths_[SoundEffectId::kResourceGoldCollect] = {
      base + "resources/gold_collect.mp3"};
  effect_paths_[SoundEffectId::kResourceElixirCollect] = {
      base + "resources/elixir_collect.mp3"};
  effect_paths_[SoundEffectId::kResourceGemCollect] = {
      base + "resources/gem_collect.mp3"};

  // ==================== 野蛮人音效（支持多变体随机播放）====================
  effect_paths_[SoundEffectId::kBarbarianDeploy] = {
      base + "units/barbarian/deploy.mp3"};
  effect_paths_[SoundEffectId::kBarbarianAttack] = {
      base + "units/barbarian/attack_01.mp3",
      base + "units/barbarian/attack_02.mp3",
      base + "units/barbarian/attack_03.mp3",
      base + "units/barbarian/attack_04.mp3"};
  effect_paths_[SoundEffectId::kBarbarianDeath] = {
      base + "units/barbarian/death_01.mp3",
      base + "units/barbarian/death_02.mp3"};
  effect_paths_[SoundEffectId::kBarbarianMove] = {
      base + "units/barbarian/move.mp3"};

  // ==================== 弓箭手音效 ====================
  effect_paths_[SoundEffectId::kArcherDeploy] = {
      base + "units/archer/deploy.mp3"};
  effect_paths_[SoundEffectId::kArcherAttack] = {
      base + "units/archer/attack_01.mp3",
      base + "units/archer/attack_02.mp3"};
  effect_paths_[SoundEffectId::kArcherDeath] = {
      base + "units/archer/death.mp3"};

  // ==================== 巨人音效 ====================
  effect_paths_[SoundEffectId::kGiantDeploy] = {
      base + "units/giant/deploy.mp3"};
  effect_paths_[SoundEffectId::kGiantAttack] = {
      base + "units/giant/attack_01.mp3",
      base + "units/giant/attack_02.mp3"};
  effect_paths_[SoundEffectId::kGiantHit] = {
      base + "units/giant/hit_01.mp3",
      base + "units/giant/hit_02.mp3"};
  effect_paths_[SoundEffectId::kGiantDeath] = {
      base + "units/giant/death.mp3"};

  // ==================== 哥布林音效 ====================
  effect_paths_[SoundEffectId::kGoblinDeploy] = {
      base + "units/goblin/deploy.mp3"};
  effect_paths_[SoundEffectId::kGoblinAttack] = {
      base + "units/goblin/attack_01.mp3",
      base + "units/goblin/attack_02.mp3",
      base + "units/goblin/attack_03.mp3"};
  effect_paths_[SoundEffectId::kGoblinDeath] = {
      base + "units/goblin/death.mp3"};

  // ==================== 炸弹人音效 ====================
  effect_paths_[SoundEffectId::kWallBreakerAttack] = {
      base + "units/wall_breaker/attack.mp3"};
  effect_paths_[SoundEffectId::kWallBreakerDeath] = {
      base + "units/wall_breaker/death.mp3"};

  // ==================== 防御建筑音效 ====================
  effect_paths_[SoundEffectId::kCannonPickup] = {
      base + "buildings/defense/cannon_pickup.mp3"};
  effect_paths_[SoundEffectId::kCannonPlace] = {
      base + "buildings/defense/cannon_place.mp3"};
  effect_paths_[SoundEffectId::kCannonFire] = {
      base + "buildings/defense/cannon_fire.mp3"};
  effect_paths_[SoundEffectId::kArcherTowerPickup] = {
      base + "buildings/defense/archer_tower_pickup.mp3"};
  effect_paths_[SoundEffectId::kArcherTowerPlace] = {
      base + "buildings/defense/archer_tower_place.mp3"};
  effect_paths_[SoundEffectId::kWallPickup] = {
      base + "buildings/defense/wall_pickup.mp3"};
  effect_paths_[SoundEffectId::kWallPlace] = {
      base + "buildings/defense/wall_place.mp3"};

  // ==================== 资源建筑音效 ====================
  effect_paths_[SoundEffectId::kGoldMinePickup] = {
      base + "buildings/resource/goldmine_pickup.mp3"};
  effect_paths_[SoundEffectId::kGoldMinePlace] = {
      base + "buildings/resource/goldmine_place.mp3"};
  effect_paths_[SoundEffectId::kElixirStoragePickup] = {
      base + "buildings/resource/elixir_storage_pickup.mp3"};
  effect_paths_[SoundEffectId::kElixirStoragePlace] = {
      base + "buildings/resource/elixir_storage_place.mp3"};

  // ==================== 通用建筑音效 ====================
  effect_paths_[SoundEffectId::kBuildingPickup] = {
      base + "buildings/general/pickup.mp3"};
  effect_paths_[SoundEffectId::kBuildingConstruct] = {
      base + "buildings/general/construct.mp3"};
  effect_paths_[SoundEffectId::kBuildingFinished] = {
      base + "buildings/general/finished.mp3"};
  effect_paths_[SoundEffectId::kBuildingDestroyed] = {
      base + "buildings/general/destroyed.mp3"};
  effect_paths_[SoundEffectId::kBuilderHutPickup] = {
      base + "buildings/general/builder_hut_pickup.mp3"};
  effect_paths_[SoundEffectId::kBuilderHutPlace] = {
      base + "buildings/general/builder_hut_place.mp3"};
}

std::string AudioConfig::GetMusicPath(MusicId music_id) const {
  auto it = music_paths_.find(music_id);
  if (it != music_paths_.end()) {
    return it->second;
  }
  return "";
}

std::string AudioConfig::GetEffectPath(SoundEffectId effect_id) const {
  auto it = effect_paths_.find(effect_id);
  if (it != effect_paths_.end() && !it->second.empty()) {
    // 如果有多个变体，随机选择一个
    if (it->second.size() == 1) {
      return it->second[0];
    }
    int index = std::rand() % static_cast<int>(it->second.size());
    return it->second[index];
  }
  return "";
}

std::vector<std::string> AudioConfig::GetEffectPaths(
    SoundEffectId effect_id) const {
  auto it = effect_paths_.find(effect_id);
  if (it != effect_paths_.end()) {
    return it->second;
  }
  return {};
}

std::vector<std::string> AudioConfig::GetAllEffectPaths() const {
  std::vector<std::string> all_paths;
  for (const auto& pair : effect_paths_) {
    for (const auto& path : pair.second) {
      all_paths.push_back(path);
    }
  }
  return all_paths;
}

std::vector<std::string> AudioConfig::GetAllMusicPaths() const {
  std::vector<std::string> all_paths;
  for (const auto& pair : music_paths_) {
    all_paths.push_back(pair.second);
  }
  return all_paths;
}
