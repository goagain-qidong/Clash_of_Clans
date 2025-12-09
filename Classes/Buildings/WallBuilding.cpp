/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 城墙建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/
#include "WallBuilding.h"
USING_NS_CC;

WallBuilding* WallBuilding::create(int level)
{
    WallBuilding* building = new (std::nothrow) WallBuilding();
    if (building && building->init(level))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool WallBuilding::init(int level)
{
    _level = std::max(1, std::min(level, getMaxLevel()));
    _gridSize = cocos2d::Size(1, 1); // 城墙占用1x1网格
    
    std::string imageFile = getImageFile();
    if (!Sprite::initWithFile(imageFile))
        return false;
    
    // 设置锚点和缩放
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.6f); // 城墙较小，缩放为0.6
    this->setName(getDisplayName());
    
    return true;
}

std::string WallBuilding::getDisplayName() const
{
    return "城墙 Lv." + std::to_string(_level);
}

int WallBuilding::getUpgradeCost() const
{
    // 城墙升级费用表（16级）
    static const int costs[] = {
        0,      // Level 0 (无效)
        50,     // Level 1
        1000,   // Level 2
        5000,   // Level 3
        10000,  // Level 4
        30000,  // Level 5
        75000,  // Level 6
        200000, // Level 7
        500000, // Level 8
        1000000,// Level 9
        1500000,// Level 10
        2000000,// Level 11
        2500000,// Level 12
        3000000,// Level 13
        4000000,// Level 14
        5000000,// Level 15
        6000000 // Level 16
    };
    
    if (_level < 1 || _level > 16)
        return 0;
    
    return costs[_level];
}

float WallBuilding::getUpgradeTime() const
{
    // 🟢 修改后：直接返回 10.0f，忽略之前的数组配置
    if (_level < 1 || _level >= getMaxLevel())
        return 0.0f; // 满级或无效等级不需要时间

    return 10.0f;
}

std::string WallBuilding::getBuildingDescription() const
{
    return StringUtils::format("生命值: %d", getHitPoints());
}

std::string WallBuilding::getImageFile() const
{
    return getImageForLevel(_level);
}

std::string WallBuilding::getImageForLevel(int level) const
{
    if (level < 1 || level > 16)
        level = 1;
    
    return "buildings/Wall/Wall" + std::to_string(level) + ".png";
}

int WallBuilding::getHitPoints() const
{
    // 城墙生命值表（16级）
    static const int hitPoints[] = {
        0,      // Level 0 (无效)
        300,    // Level 1
        500,    // Level 2
        700,    // Level 3
        900,    // Level 4
        1400,   // Level 5
        2000,   // Level 6
        2500,   // Level 7
        3000,   // Level 8
        4000,   // Level 9
        5500,   // Level 10
        7000,   // Level 11
        8500,   // Level 12
        10000,  // Level 13
        12000,  // Level 14
        14000,  // Level 15
        17000   // Level 16
    };
    
    if (_level < 1 || _level > 16)
        return 0;
    
    return hitPoints[_level];
}

void WallBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    CCLOG("WallBuilding upgraded to level %d, HP: %d", _level, getHitPoints());
}
