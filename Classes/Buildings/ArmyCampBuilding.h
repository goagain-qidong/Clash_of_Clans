/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyCampBuilding.h
 * File Function: 军营建筑类（存放士兵的营地）
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseBuilding.h"

/**
 * @class ArmyCampBuilding
 * @brief 军营建筑类，4x4网格，用于存放训练好的士兵
 */
class ArmyCampBuilding : public BaseBuilding
{
public:
    static ArmyCampBuilding* create(int level = 1);
    
    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kArmy; }
    virtual std::string getDisplayName() const override;
    virtual int getMaxLevel() const override { return 13; }
    virtual int getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kElixir; }
    virtual float getUpgradeTime() const override;
    virtual std::string getBuildingDescription() const override;
    virtual std::string getImageFile() const override;
    
    // ==================== 军营特有功能 ====================
    /** @brief 获取当前等级的容纳人口数 */
    int getHousingSpace() const;

protected:
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    ArmyCampBuilding() = default;
};
