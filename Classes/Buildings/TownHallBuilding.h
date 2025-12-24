/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TownHallBuilding.h
 * File Function: 大本营建筑类
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef TOWN_HALL_BUILDING_H_
#define TOWN_HALL_BUILDING_H_

#include "BaseBuilding.h"

#include <vector>

/**
 * @class TownHallConfig
 * @brief 大本营等级配置管理器（单例）
 */
class TownHallConfig
{
public:
    /**
     * @struct LevelData
     * @brief 大本营等级数据
     */
    struct LevelData
    {
        int level;               ///< 等级
        int hitpoints;           ///< 生命值
        int upgradeCost;         ///< 升级费用（金币）
        float upgradeTime;       ///< 升级时间（秒）
        int experienceGained;    ///< 升级后获得的经验值
        int maxBuildings;        ///< 最大建筑数量
        int maxTraps;            ///< 最大陷阱数量
        std::string imageFile;   ///< 图片路径
        std::string description; ///< 描述
    };

    /**
     * @brief 获取单例实例
     * @return TownHallConfig* 配置实例指针
     */
    static TownHallConfig* getInstance();

    /**
     * @brief 获取指定等级数据
     * @param level 等级
     * @return const LevelData* 等级数据指针
     */
    const LevelData* getLevel(int level) const;

    /**
     * @brief 获取下一等级数据
     * @param currentLevel 当前等级
     * @return const LevelData* 下一等级数据指针
     */
    const LevelData* getNextLevel(int currentLevel) const;

    /** @brief 获取最大等级 */
    int getMaxLevel() const { return static_cast<int>(_levels.size()); }

    /**
     * @brief 检查是否可以升级
     * @param currentLevel 当前等级
     * @return bool 是否可以升级
     */
    bool canUpgrade(int currentLevel) const;

    /**
     * @brief 获取升级费用
     * @param currentLevel 当前等级
     * @return int 升级费用
     */
    int getUpgradeCost(int currentLevel) const;

    /**
     * @brief 获取建筑最大等级限制
     * @param townHallLevel 大本营等级
     * @param buildingName 建筑名称
     * @return int 最大等级
     */
    int getMaxBuildingLevel(int townHallLevel, const std::string& buildingName) const;

    /**
     * @brief 检查建筑是否已解锁
     * @param townHallLevel 大本营等级
     * @param buildingName 建筑名称
     * @return bool 是否已解锁
     */
    bool isBuildingUnlocked(int townHallLevel, const std::string& buildingName) const;

private:
    TownHallConfig();
    void initialize();
    std::vector<LevelData> _levels;  ///< 等级数据列表
};

/**
 * @class TownHallBuilding
 * @brief 大本营建筑类
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

    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const override { return BuildingType::kTownHall; }

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const override;

    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const override;

    /** @brief 获取升级费用 */
    virtual int getUpgradeCost() const override;

    /** @brief 获取升级资源类型 */
    virtual ResourceType getUpgradeCostType() const override { return kGold; }

    /** @brief 获取升级时间 */
    virtual float getUpgradeTime() const override;

    /** @brief 获取升级信息 */
    virtual std::string getUpgradeInfo() const override;

    /** @brief 获取当前图片文件 */
    virtual std::string getImageFile() const override;

    /** @brief 检查是否可以升级 */
    virtual bool canUpgrade() const override;

protected:
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual void updateAppearance() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    TownHallBuilding() = default;
};

#endif // TOWN_HALL_BUILDING_H_