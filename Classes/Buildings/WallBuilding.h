/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.h
 * File Function: 城墙建筑类
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseBuilding.h"

/**
 * @class WallBuilding
 * @brief 城墙建筑类，1x1网格的防御建筑
 */
class WallBuilding : public BaseBuilding
{
public:
    static WallBuilding* create(int level = 1);
    
    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kWall; }
    virtual std::string getDisplayName() const override;
    virtual int getMaxLevel() const override { return 16; }
    virtual int getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kGold; }
    virtual float getUpgradeTime() const override;
    virtual std::string getBuildingDescription() const override;
    virtual std::string getImageFile() const override;
    
    // ==================== 城墙特有功能 ====================
    /** @brief 获取当前等级的生命值 */
    int getHitPoints() const;

protected:
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    WallBuilding() = default;
};
