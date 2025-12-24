/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TownHallBuilding.h
 * File Function: 大本营建筑类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#ifndef TOWN_HALL_BUILDING_H_
#define TOWN_HALL_BUILDING_H_

#include "BaseBuilding.h"

/**
 * @class TownHallBuilding
 * @brief 大本营建筑类
 * @note 使用数据驱动架构，配置数据统一由 BaseBuilding::getStaticConfig() 管理
 */
class TownHallBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建大本营建筑
     * @param level 建筑等级
     * @return TownHallBuilding* 大本营指针
     */
    static TownHallBuilding* create(int level = 1);

    // 以下方法直接使用基类的配置驱动实现，无需重写：
    // - getMaxLevel()
    // - getUpgradeCost()
    // - getUpgradeTime()
    // - getMaxHitpoints()
    // - getImageFile()

    /** @brief 获取升级信息 */
    virtual std::string getUpgradeInfo() const override;

    /** @brief 检查是否可以升级 */
    virtual bool canUpgrade() const override;

    /**
     * @brief 获取建筑最大等级限制
     * @param buildingName 建筑名称
     * @return int 最大等级
     */
    int getMaxBuildingLevel(const std::string& buildingName) const;

    /**
     * @brief 检查建筑是否已解锁
     * @param buildingName 建筑名称
     * @return bool 是否已解锁
     */
    bool isBuildingUnlocked(const std::string& buildingName) const;

protected:
    /**
     * @brief 初始化大本营
     * @param level 初始等级
     * @return bool 是否成功
     */
    virtual bool init(int level) override;

    /** @brief 升级时调用 */
    virtual void onLevelUp() override;

    /** @brief 更新外观 */
    virtual void updateAppearance() override;

private:
    TownHallBuilding() = default;
};

#endif // TOWN_HALL_BUILDING_H_