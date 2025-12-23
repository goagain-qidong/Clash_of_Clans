/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GiantUnit.cpp
 * File Function: 巨人单位类实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#include "GiantUnit.h"

#include "Unit/CombatStats.h"

USING_NS_CC;

GiantUnit* GiantUnit::create(int level)
{
    GiantUnit* unit = new (std::nothrow) GiantUnit();
    if (unit && unit->init(level))
    {
        unit->autorelease();
        return unit;
    }
    CC_SAFE_DELETE(unit);
    return nullptr;
}

bool GiantUnit::init(int level)
{
    if (!BaseUnit::init(level))
        return false;

    // 设置巨人特有属性
    _moveSpeed   = 60.0f; // 巨人移动慢
    _combatStats = UnitConfig::getGiant(level);

    return true;
}

void GiantUnit::loadAnimations()
{
    // 加载图集
    SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/giant/giant.plist");

    // 创建精灵
    _sprite = Sprite::createWithSpriteFrameName("giant38.0.png");
    if (_sprite)
    {
        this->addChild(_sprite);
        _sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
    }

    // 加载跑步动画
    addAnimFromFrames("giant", "run_down_right", 1, 12, 0.12f);
    addAnimFromFrames("giant", "run_right", 13, 24, 0.12f);
    addAnimFromFrames("giant", "run_up_right", 25, 36, 0.12f);

    // 加载待机动画
    addAnimFromFrames("giant", "idle_down_right", 37, 37, 1.0f);
    addAnimFromFrames("giant", "idle_right", 38, 38, 1.0f);
    addAnimFromFrames("giant", "idle_up_right", 39, 39, 1.0f);

    // 加载第一套攻击动画
    addAnimFromFrames("giant", "attack_down_right", 46, 54, 0.11f);
    addAnimFromFrames("giant", "attack_right", 55, 63, 0.11f);
    addAnimFromFrames("giant", "attack_up_right", 64, 71, 0.11f);

    // 加载第二套攻击动画
    addAnimFromFrames("giant", "attack2_down_right", 72, 80, 0.11f);
    addAnimFromFrames("giant", "attack2_right", 81, 89, 0.11f);
    addAnimFromFrames("giant", "attack2_up_right", 90, 97, 0.11f);

    // 加载死亡动画
    addAnimFromFrames("giant", "death", 99, 100, 0.5f);

    // 播放待机动画
    playAnimation(UnitAction::kIdle, UnitDirection::kRight);
}