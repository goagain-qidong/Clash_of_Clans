/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GoblinUnit.cpp
 * File Function: 哥布林单位类实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "GoblinUnit.h"

#include "Audio/AudioManager.h"
#include "Unit/CombatStats.h"

USING_NS_CC;

GoblinUnit* GoblinUnit::create(int level) {
  GoblinUnit* unit = new (std::nothrow) GoblinUnit();
  if (unit && unit->init(level)) {
    unit->autorelease();
    return unit;
  }
  CC_SAFE_DELETE(unit);
  return nullptr;
}

bool GoblinUnit::init(int level) {
  if (!BaseUnit::init(level)) {
    return false;
  }

  // 设置哥布林特有属性
  _moveSpeed = 150.0f;  // 哥布林移动快
  _combatStats = UnitConfig::getGoblin(level);

  // 播放部署音效
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kGoblinDeploy);

  return true;
}

void GoblinUnit::loadAnimations() {
  // 加载图集
  SpriteFrameCache::getInstance()->addSpriteFramesWithFile(
      "units/goblin/goblin.plist");

  // 创建精灵
  _sprite = Sprite::createWithSpriteFrameName("goblin26.0.png");
  if (_sprite) {
    this->addChild(_sprite);
    _sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
  }

  // 加载跑步动画
  addAnimFromFrames("goblin", "run_down_right", 1, 8, 0.09f);
  addAnimFromFrames("goblin", "run_right", 9, 16, 0.09f);
  addAnimFromFrames("goblin", "run_up_right", 17, 24, 0.09f);

  // 加载待机动画
  addAnimFromFrames("goblin", "idle_down_right", 25, 25, 1.0f);
  addAnimFromFrames("goblin", "idle_right", 26, 26, 1.0f);
  addAnimFromFrames("goblin", "idle_up_right", 27, 27, 1.0f);

  // 加载攻击动画
  addAnimFromFrames("goblin", "attack_down_right", 28, 31, 0.08f);
  addAnimFromFrames("goblin", "attack_right", 32, 36, 0.08f);
  addAnimFromFrames("goblin", "attack_up_right", 37, 40, 0.08f);

  // 加载死亡动画
  addAnimFromFrames("goblin", "death", 41, 42, 0.5f);

  // 播放待机动画
  playAnimation(UnitAction::kIdle, UnitDirection::kRight);
}

void GoblinUnit::onAttackBefore() {
  // 播放攻击音效（随机变体）
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kGoblinAttack);
}

void GoblinUnit::onDeathBefore() {
  // 播放死亡音效
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kGoblinDeath);
}