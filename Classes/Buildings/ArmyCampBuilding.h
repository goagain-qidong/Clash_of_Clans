/****************************************************************
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
 * @brief 军营建筑类，4x4网格，用于存放训练好的士兵
 */
class ArmyCampBuilding : public BaseBuilding
{
public:
    static ArmyCampBuilding* create(int level = 1);
    
    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kArmyCamp; }
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
    
    /** @brief 添加训练好的小兵到军营显示 */
    void addTroopDisplay(UnitType type);
    
    /** @brief 移除一个小兵显示（部署时调用） */
    void removeTroopDisplay(UnitType type);
    
    /** @brief 清空所有小兵显示 */
    void clearTroopDisplays();
    
    /** @brief 更新小兵显示位置（军营移动时调用） */
    void updateTroopPositions();
    
    /** @brief 根据士兵库存刷新军营显示（战斗结束后调用） */
    void refreshDisplayFromInventory();

protected:
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    ArmyCampBuilding() = default;
    
    // 存储显示的小兵
    std::vector<cocos2d::Node*> _troopSprites;
    
    // 获取小兵应该显示的位置（相对于军营）
    cocos2d::Vec2 getTroopDisplayPosition(int index) const;
};

#endif  // ARMY_CAMP_BUILDING_H_
