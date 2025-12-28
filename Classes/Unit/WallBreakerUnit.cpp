/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBreakerUnit.cpp
 * File Function: 炸弹人单位类实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "WallBreakerUnit.h"

#include "Audio/AudioManager.h"
#include "Buildings/BaseBuilding.h"
#include "Unit/CombatStats.h"

USING_NS_CC;

WallBreakerUnit* WallBreakerUnit::create(int level) {
  WallBreakerUnit* unit = new (std::nothrow) WallBreakerUnit();
  if (unit && unit->init(level)) {
    unit->autorelease();
    return unit;
  }
  CC_SAFE_DELETE(unit);
  return nullptr;
}

bool WallBreakerUnit::init(int level) {
  if (!BaseUnit::init(level)) {
    return false;
  }

  // 设置炸弹人特有属性
  _moveSpeed = 120.0f;
  _combatStats = UnitConfig::getWallBreaker(level);

  // 注：炸弹人没有专门的部署音效，使用死亡/爆炸音效更有特色

  return true;
}

void WallBreakerUnit::loadAnimations() {
  // 加载图集
  SpriteFrameCache::getInstance()->addSpriteFramesWithFile(
      "units/wall_breaker/wall_breaker.plist");

  // 创建精灵
  _sprite = Sprite::createWithSpriteFrameName("wall_breaker21.0.png");
  if (_sprite) {
    this->addChild(_sprite);
    _sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
  }

  // 加载跑步动画
  addAnimFromFrames("wall_breaker", "run_down_right", 49, 56, 0.10f);
  addAnimFromFrames("wall_breaker", "run_right", 41, 48, 0.10f);
  addAnimFromFrames("wall_breaker", "run_up_right", 33, 40, 0.10f);

  // 加载待机动画
  addAnimFromFrames("wall_breaker", "idle_down_right", 27, 27, 1.0f);
  addAnimFromFrames("wall_breaker", "idle_right", 21, 21, 1.0f);
  addAnimFromFrames("wall_breaker", "idle_up_right", 20, 20, 1.0f);

  // 加载死亡动画（爆炸）
  addAnimFromFrames("wall_breaker", "death", 2, 1, 0.5f);

  // 播放待机动画
  playAnimation(UnitAction::kIdle, UnitDirection::kRight);
}

void WallBreakerUnit::onAttackBefore() {
  // 炸弹人攻击（爆炸）音效
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kWallBreakerAttack);
}

void WallBreakerUnit::onDeathBefore() {
  // 炸弹人特殊死亡逻辑：爆炸时对目标造成范围伤害
  if (_currentTarget && !_currentTarget->isDestroyed()) {
    // 炸弹人对城墙造成40倍伤害
    float damage = _combatStats.damage;
    if (_currentTarget->getBuildingType() == BuildingType::kWall) {
      damage *= 40.0f;
      CCLOG("💣 炸弹人对城墙造成 %.0f 点伤害！", damage);
    } else {
      // 对非城墙建筑造成普通伤害
      CCLOG("💣 炸弹人对建筑造成 %.0f 点伤害！", damage);
    }
    _currentTarget->takeDamage(static_cast<int>(damage));
  }

  // 播放爆炸/死亡音效
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kWallBreakerDeath);

  // 播放爆炸动画
  playAnimation(UnitAction::kDeath, _currentDir);

  // 创建爆炸视觉效果
  createExplosionEffect();

  // 延迟后移除自身
  auto removeAction =
      Sequence::create(DelayTime::create(0.5f), RemoveSelf::create(), nullptr);
  this->runAction(removeAction);

  CCLOG("💣 炸弹人爆炸！");
}

void WallBreakerUnit::createExplosionEffect() {
  // 创建简单的爆炸效果（缩放 + 淡出的圆形）
  auto explosion = DrawNode::create();
  explosion->drawSolidCircle(Vec2::ZERO, 50.0f, 0, 30,
                             Color4F(1.0f, 0.5f, 0.0f, 0.8f));
  explosion->setPosition(this->getPosition());

  if (this->getParent()) {
    this->getParent()->addChild(explosion, 1000);

    auto scaleUp = ScaleTo::create(0.2f, 2.0f);
    auto fadeOut = FadeOut::create(0.3f);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(scaleUp, fadeOut, remove, nullptr);
    explosion->runAction(sequence);
  }
}