/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyCampBuilding.cpp
 * File Function: 军营建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/
#include "ArmyCampBuilding.h"
USING_NS_CC;

ArmyCampBuilding* ArmyCampBuilding::create(int level)
{
    ArmyCampBuilding* building = new (std::nothrow) ArmyCampBuilding();
    if (building && building->init(level))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool ArmyCampBuilding::init(int level)
{
    _level = std::max(1, std::min(level, getMaxLevel()));
    _gridSize = cocos2d::Size(4, 4); // 军营占用4x4网格
    
    std::string imageFile = getImageFile();
    if (!Sprite::initWithFile(imageFile))
        return false;
    
    // 设置锚点和缩放
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);
    this->setName(getDisplayName());
    
    return true;
}

std::string ArmyCampBuilding::getDisplayName() const
{
    return "军营 Lv." + std::to_string(_level);
}

int ArmyCampBuilding::getUpgradeCost() const
{
    // 军营升级费用表（13级）
    static const int costs[] = {
        0,       // Level 0 (无效)
        250,     // Level 1
        2500,    // Level 2
        10000,   // Level 3
        100000,  // Level 4
        250000,  // Level 5
        750000,  // Level 6
        2250000, // Level 7
        6000000, // Level 8
        7500000, // Level 9
        9000000, // Level 10
        10500000,// Level 11
        12000000,// Level 12
        14000000 // Level 13
    };
    
    if (_level < 1 || _level > 13)
        return 0;
    
    return costs[_level];
}

float ArmyCampBuilding::getUpgradeTime() const
{
    // 军营升级时间（秒）
    static const float times[] = {
        0,      // Level 0 (无效)
        0,      // Level 1 (即时)
        900,    // Level 2 (15分钟)
        3600,   // Level 3 (1小时)
        28800,  // Level 4 (8小时)
        86400,  // Level 5 (1天)
        172800, // Level 6 (2天)
        259200, // Level 7 (3天)
        345600, // Level 8 (4天)
        432000, // Level 9 (5天)
        518400, // Level 10 (6天)
        604800, // Level 11 (7天)
        691200, // Level 12 (8天)
        777600  // Level 13 (9天)
    };
    
    if (_level < 1 || _level > 13)
        return 0;
    
    return times[_level];
}

std::string ArmyCampBuilding::getBuildingDescription() const
{
    return StringUtils::format("容纳人口: %d", getHousingSpace());
}

std::string ArmyCampBuilding::getImageFile() const
{
    return getImageForLevel(_level);
}

std::string ArmyCampBuilding::getImageForLevel(int level) const
{
    if (level < 1 || level > 13)
        level = 1;
    
    return "buildings/ArmyCamp/Army_Camp" + std::to_string(level) + ".png";
}

int ArmyCampBuilding::getHousingSpace() const
{
    // 军营容纳人口表（13级）
    static const int housingSpace[] = {
        0,   // Level 0 (无效)
        20,  // Level 1
        30,  // Level 2
        35,  // Level 3
        40,  // Level 4
        45,  // Level 5
        50,  // Level 6
        55,  // Level 7
        60,  // Level 8
        65,  // Level 9
        70,  // Level 10
        75,  // Level 11
        80,  // Level 12
        85   // Level 13
    };
    
    if (_level < 1 || _level > 13)
        return 0;
    
    return housingSpace[_level];
}

void ArmyCampBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    CCLOG("ArmyCampBuilding upgraded to level %d, Housing Space: %d", _level, getHousingSpace());
}
