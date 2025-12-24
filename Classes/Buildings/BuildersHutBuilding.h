/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildersHutBuilding.h
 * File Function: 建筑工人小屋类
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "BaseBuilding.h"

/**
 * @class BuildersHutBuilding
 * @brief 建筑工人小屋类 - 2x2网格，提供建筑工人
 */
class BuildersHutBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建建筑工人小屋
     * @param level 建筑等级
     * @return BuildersHutBuilding* 建筑工人小屋指针
     */
    static BuildersHutBuilding* create(int level = 1);

    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const override { return BuildingType::kDecoration; }

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const override;

    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const override { return 7; }

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

    /** @brief 是否有可用的建筑工人 */
    bool hasAvailableBuilder() const { return _isBuilderAvailable; }

    /**
     * @brief 设置建筑工人可用状态
     * @param available 是否可用
     */
    void setBuilderAvailable(bool available) { _isBuilderAvailable = available; }

protected:
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    BuildersHutBuilding() = default;
    bool _isBuilderAvailable = true;  ///< 建筑工人是否可用
};
