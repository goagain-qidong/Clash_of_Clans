/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildersHutBuilding.cpp
 * File Function: 建筑工人小屋类实现
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/
#include "BuildersHutBuilding.h"
USING_NS_CC;

BuildersHutBuilding* BuildersHutBuilding::create(int level)
{
    BuildersHutBuilding* building = new (std::nothrow) BuildersHutBuilding();
    if (building && building->init(level))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool BuildersHutBuilding::init(int level)
{
    _level = std::max(1, std::min(level, getMaxLevel()));
    _gridSize = cocos2d::Size(2, 2); // 建筑工人小屋占用2x2网格
    _isBuilderAvailable = true;
    
    std::string imageFile = getImageFile();
    if (!Sprite::initWithFile(imageFile))
        return false;
    
    // 设置锚点和缩放
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.7f);
    this->setName(getDisplayName());
    
    return true;
}

std::string BuildersHutBuilding::getDisplayName() const
{
    return "建筑工人小屋 Lv." + std::to_string(_level);
}

int BuildersHutBuilding::getUpgradeCost() const
{
    // 建筑工人小屋升级费用表（7级）
    // 注意：建筑工人小屋通常不能升级，这里提供数据仅供参考
    static const int costs[] = {
        0,       // Level 0 (无效)
        0,       // Level 1 (购买宝石解锁)
        0,       // Level 2 (购买宝石解锁)
        0,       // Level 3 (购买宝石解锁)
        0,       // Level 4 (购买宝石解锁)
        0,       // Level 5 (购买宝石解锁)
        0,       // Level 6 (购买宝石解锁)
        0        // Level 7 (购买宝石解锁)
    };
    
    if (_level < 1 || _level > 7)
        return 0;
    
    return costs[_level];
}

float BuildersHutBuilding::getUpgradeTime() const
{
    // 建筑工人小屋不需要升级时间（通过宝石解锁）
    return 0.0f;
}

std::string BuildersHutBuilding::getBuildingDescription() const
{
    return "建筑工人状态: ";
}

std::string BuildersHutBuilding::getImageFile() const
{
    return getImageForLevel(_level);
}

std::string BuildersHutBuilding::getImageForLevel(int level) const
{
    if (level < 1 || level > 7)
        level = 1;
    
    return "buildings/BuildersHut/Builders_Hut" + std::to_string(level) + ".png";
}

void BuildersHutBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    CCLOG("BuildersHutBuilding level: %d, Available: %d", _level, _isBuilderAvailable);
}
