/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BarbarianUnit.cpp
 * File Function: 野蛮人单位类实现
 * Author:        薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "BarbarianUnit.h"

#include "Audio/AudioManager.h"
#include "Unit/CombatStats.h"

USING_NS_CC;

BarbarianUnit* BarbarianUnit::create(int level) {
  BarbarianUnit* unit = new (std::nothrow) BarbarianUnit();
  if (unit && unit->init(level)) {
    unit->autorelease();
    return unit;
  }
  CC_SAFE_DELETE(unit);
  return nullptr;
}

bool BarbarianUnit::init(int level) {
  if (!BaseUnit::init(level)) {
    return false;
  }

  // 设置野蛮人特有属性
  _moveSpeed = 100.0f;
  _combatStats = UnitConfig::getBarbarian(level);

  // 播放部署音效
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kBarbarianDeploy);

  return true;
}

void BarbarianUnit::loadAnimations() {
  // 加载图集
  SpriteFrameCache::getInstance()->addSpriteFramesWithFile(
      "units/barbarian/barbarian.plist");

  // 创建精灵
  _sprite = Sprite::createWithSpriteFrameName("barbarian25.0.png");
  if (_sprite) {
    this->addChild(_sprite);
    _sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
  }

  // 加载跑步动画
  addAnimFromFrames("barbarian", "run_down_right", 1, 8, 0.1f);
  addAnimFromFrames("barbarian", "run_right", 9, 16, 0.1f);
  addAnimFromFrames("barbarian", "run_up_right", 17, 24, 0.1f);

  // 加载待机动画
  addAnimFromFrames("barbarian", "idle_down_right", 25, 25, 1.0f);
  addAnimFromFrames("barbarian", "idle_right", 26, 26, 1.0f);
  addAnimFromFrames("barbarian", "idle_up_right", 27, 27, 1.0f);

  // 加载攻击动画
  addAnimFromFrames("barbarian", "attack_down_right", 31, 38, 0.1f);
  addAnimFromFrames("barbarian", "attack_right", 39, 46, 0.1f);
  addAnimFromFrames("barbarian", "attack_up_right", 47, 54, 0.1f);

  // 加载第二套攻击动画
  addAnimFromFrames("barbarian", "attack2_down_right", 55, 65, 0.09f);
  addAnimFromFrames("barbarian", "attack2_right", 66, 76, 0.09f);
  addAnimFromFrames("barbarian", "attack2_up_right", 77, 87, 0.09f);

  // 加载死亡动画
  addAnimFromFrames("barbarian", "death", 175, 176, 0.5f);

  // 播放待机动画
  playAnimation(UnitAction::kIdle, UnitDirection::kRight);
}

void BarbarianUnit::onAttackBefore() {
  // 播放攻击音效（随机变体）
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kBarbarianAttack);
}

void BarbarianUnit::onDeathBefore() {
  // 播放死亡音效（随机变体）
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kBarbarianDeath);
}