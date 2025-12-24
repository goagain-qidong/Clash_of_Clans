/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyCampBuilding.h
 * File Function: 军营建筑类（存放士兵的营地）
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef ARMY_CAMP_BUILDING_H_
#define ARMY_CAMP_BUILDING_H_

#include "BaseBuilding.h"
#include "Unit/UnitTypes.h"

#include <vector>

/**
 * @class ArmyCampBuilding
 * @brief 军营建筑类 - 4x4网格，用于存放训练好的士兵
 */
class ArmyCampBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建军营建筑
     * @param level 建筑等级
     * @return ArmyCampBuilding* 军营指针
     */
    static ArmyCampBuilding* create(int level = 1);

    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const override { return BuildingType::kArmyCamp; }

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const override;

    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const override { return 13; }

    /** @brief 获取升级费用 */
    virtual int getUpgradeCost() const override;

    /** @brief 获取升级资源类型 */
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kElixir; }

    /** @brief 获取升级时间 */
    virtual float getUpgradeTime() const override;

    /** @brief 获取建筑描述 */
    virtual std::string getBuildingDescription() const override;

    /** @brief 获取当前图片文件 */
    virtual std::string getImageFile() const override;

    /** @brief 获取当前等级的容纳人口数 */
    int getHousingSpace() const;

    /**
     * @brief 添加训练好的小兵到军营显示
     * @param type 兵种类型
     */
    void addTroopDisplay(UnitType type);

    /**
     * @brief 移除一个小兵显示
     * @param type 兵种类型
     */
    void removeTroopDisplay(UnitType type);

    /** @brief 清空所有小兵显示 */
    void clearTroopDisplays();

    /** @brief 更新小兵显示位置 */
    void updateTroopPositions();

    /** @brief 根据士兵库存刷新军营显示 */
    void refreshDisplayFromInventory();

protected:
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    ArmyCampBuilding() = default;

    std::vector<cocos2d::Node*> _troopSprites;  ///< 存储显示的小兵

    /**
     * @brief 获取小兵应该显示的位置
     * @param index 小兵索引
     * @return cocos2d::Vec2 显示位置
     */
    cocos2d::Vec2 getTroopDisplayPosition(int index) const;
};

#endif  // ARMY_CAMP_BUILDING_H_
