/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArcherUnit.cpp
 * File Function: 弓箭手单位类实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "ArcherUnit.h"

#include "Audio/AudioManager.h"
#include "Unit/CombatStats.h"

USING_NS_CC;

ArcherUnit* ArcherUnit::create(int level) {
  ArcherUnit* unit = new (std::nothrow) ArcherUnit();
  if (unit && unit->init(level)) {
    unit->autorelease();
    return unit;
  }
  CC_SAFE_DELETE(unit);
  return nullptr;
}

bool ArcherUnit::init(int level) {
  if (!BaseUnit::init(level)) {
    return false;
  }

  // 设置弓箭手特有属性
  _moveSpeed = 100.0f;
  _combatStats = UnitConfig::getArcher(level);

  // 播放部署音效
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kArcherDeploy);

  return true;
}

void ArcherUnit::loadAnimations() {
  // 弓箭手使用单独的PNG文件，不需要plist

  // 先加载一帧作为初始精灵
  auto texture = Director::getInstance()->getTextureCache()->addImage(
      "units/archer/archer_side_walk_01.png");
  if (texture) {
    auto frame = SpriteFrame::createWithTexture(
        texture,
        Rect(0, 0, texture->getContentSize().width,
             texture->getContentSize().height));
    _sprite = Sprite::createWithSpriteFrame(frame);
  }

  if (_sprite) {
    this->addChild(_sprite);
    _sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
  }

  // 加载跑步动画
  addAnimFromFiles("units/archer/", "archer_upper_walk_%02d.png", "run_up_right",
                   1, 8, 0.1f);
  addAnimFromFiles("units/archer/", "archer_side_walk_%02d.png", "run_right", 1,
                   8, 0.1f);
  addAnimFromFiles("units/archer/", "archer_under_walk_%02d.png",
                   "run_down_right", 1, 8, 0.1f);

  // 加载待机动画
  addAnimFromFiles("units/archer/", "archer_upper_walk_%02d.png",
                   "idle_up_right", 1, 3, 0.3f);
  addAnimFromFiles("units/archer/", "archer_side_walk_%02d.png", "idle_right", 1,
                   3, 0.3f);
  addAnimFromFiles("units/archer/", "archer_under_walk_%02d.png",
                   "idle_down_right", 1, 3, 0.3f);

  // 加载攻击动画
  addAnimFromFiles("units/archer/", "archer_upper_attack_%02d.png",
                   "attack_up_right", 1, 9, 0.08f);
  addAnimFromFiles("units/archer/", "archer_side_attack_%02d.png",
                   "attack_right", 1, 10, 0.08f);
  addAnimFromFiles("units/archer/", "archer_under_attack_%02d.png",
                   "attack_down_right", 1, 9, 0.08f);

  // 加载死亡动画
  addAnimFromFiles("units/archer/", "archer%d.0.png", "death", 53, 54, 0.5f);

  // 播放待机动画
  playAnimation(UnitAction::kIdle, UnitDirection::kRight);
}

void ArcherUnit::onAttackBefore() {
  // 播放攻击音效（随机变体）
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kArcherAttack);
}

void ArcherUnit::onDeathBefore() {
  // 播放死亡音效
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kArcherDeath);
}