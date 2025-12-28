/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 城墙建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "WallBuilding.h"

USING_NS_CC;

// 城墙默认缩放系数，与 ShopLayer 中的 BuildingData 保持一致
static const float kWallDefaultScale = 0.6f;

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
    if (!initWithType(BuildingType::kWall, level))
        return false;

    // 设置锚点和缩放
    // 注意：此缩放值必须与 ShopLayer::loadCategory 中城墙的 scaleFactor 保持一致
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(kWallDefaultScale);
    
    // 初始化血条UI（战斗模式下显示）
    initHealthBarUI();
    
    return true;
}

void WallBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
}
