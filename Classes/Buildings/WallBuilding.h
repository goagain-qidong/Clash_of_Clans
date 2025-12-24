/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.h
 * File Function: 城墙建筑类
 * Author:        薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "BaseBuilding.h"

/**
 * @class WallBuilding
 * @brief 城墙建筑类 - 1x1网格的防御建筑
 * @note 使用数据驱动架构，配置数据统一由 BaseBuilding::getStaticConfig() 管理
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

    // 以下方法直接使用基类的配置驱动实现，无需重写：
    // - getMaxLevel()
    // - getUpgradeCost()
    // - getUpgradeTime()
    // - getMaxHitpoints()
    // - getBuildingDescription()
    // - getImageFile()

protected:
    /**
     * @brief 初始化城墙
     * @param level 初始等级
     * @return bool 是否成功
     */
    virtual bool init(int level) override;

    /** @brief 升级时调用 */
    virtual void onLevelUp() override;

private:
    WallBuilding() = default;
};
