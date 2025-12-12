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
    
    // 记录城墙建造
    // BuildingLimitManager::getInstance()->recordBuilding("Wall");
    
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
    // 城墙升级时间（秒）
    static const float times[] = {
        0,      // Level 0 (无效)
        30,      // Level 1 (即时)
        60,     // Level 2 (1分钟)
        300,    // Level 3 (5分钟)
        900,    // Level 4 (15分钟)
        1800,   // Level 5 (30分钟)
        3600,   // Level 6 (1小时)
        7200,   // Level 7 (2小时)
        14400,  // Level 8 (4小时)
        28800,  // Level 9 (8小时)
        43200,  // Level 10 (12小时)
        86400,  // Level 11 (1天)
        172800, // Level 12 (2天)
        259200, // Level 13 (3天)
        345600, // Level 14 (4天)
        432000, // Level 15 (5天)
        518400  // Level 16 (6天)
    };

    if (_level < 1 || _level > 16)
        return 0;

    return times[_level];
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
