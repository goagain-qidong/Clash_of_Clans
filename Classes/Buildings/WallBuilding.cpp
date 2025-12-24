/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 城墙建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/12/24
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
    // 使用 initWithType 统一初始化，配置数据由基类管理
    if (!initWithType(BuildingType::kWall, level))
    {
        return false;
    }

    // 城墙特有的外观设置
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.6f);

    // 初始化血条UI
    initHealthBarUI();

    CCLOG("🧱 %s 初始化 HP: %d", getDisplayName().c_str(), getMaxHitpoints());
    return true;
}

void WallBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    CCLOG("WallBuilding upgraded to level %d, HP: %d", _level, getMaxHitpoints());
}
