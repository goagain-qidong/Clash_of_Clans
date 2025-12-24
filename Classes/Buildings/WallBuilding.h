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
 * @brief 城墙建筑类 - 1x1网格的防御建筑
 */
class WallBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建城墙建筑
     * @param level 建筑等级
     * @return WallBuilding* 城墙指针
     */
    static WallBuilding* create(int level = 1);

    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const override { return BuildingType::kWall; }

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const override;

    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const override { return 16; }

    /** @brief 获取升级费用 */
    virtual int getUpgradeCost() const override;

    /** @brief 获取升级资源类型 */
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kGold; }

    /** @brief 获取升级时间 */
    virtual float getUpgradeTime() const override;

    /** @brief 获取建筑描述 */
    virtual std::string getBuildingDescription() const override;

    /** @brief 获取当前图片文件 */
    virtual std::string getImageFile() const override;

    /** @brief 获取当前等级的生命值 */
    int getHitPoints() const;

protected:
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    WallBuilding() = default;
};
