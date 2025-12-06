/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyBuilding.h
 * File Function: 军事建筑类（兵营、训练营等）
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseBuilding.h"
/**
 * @class ArmyBuilding
 * @brief 军事建筑类，用于训练士兵
 */
class ArmyBuilding : public BaseBuilding
{
public:
    static ArmyBuilding* create(int level = 1);
    static ArmyBuilding* create(int level, const std::string& imageFile);
    virtual bool init(int level);
    virtual bool init(int level, const std::string& imageFile);
    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kArmy; }
    virtual std::string getDisplayName() const override;
    virtual int getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kElixir; }
    virtual float getUpgradeTime() const override;
    virtual int getMaxLevel() const override { return 14; }
    virtual std::string getBuildingDescription() const override;
    // ==================== 兵营特有功能 ====================
    /** @brief 获取当前可训练的士兵容量 */
    int getTrainingCapacity() const;
    /** @brief 获取训练速度加成 (百分比) */
    float getTrainingSpeedBonus() const;

protected:
    virtual void onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    std::string _customImagePath;  // 自定义图片路径（用于箭塔、炮塔等）
    std::string _customName;       // 自定义建筑名称
};