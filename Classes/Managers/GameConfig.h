/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GameConfig.h
 * File Function: 游戏配置数据
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>
#include <map>
#include <vector>

#include "ResourceManager.h"

/**
 * @struct BuildingConfigItem
 * @brief 建筑配置项
 */
struct BuildingConfigItem {
    std::string name;           ///< 建筑名称
    std::string displayName;    ///< 显示名称
    std::string iconPath;       ///< 商店图标路径
    ResourceType costType;      ///< 花费类型
    int cost;                   ///< 造价 (1级)
    int unlockTownHallLevel;    ///< 解锁需要的大本营等级
    std::vector<int> maxCountPerTHLevel;  ///< 不同大本营等级对应的最大建筑数量
    int capacityIncrease;       ///< 该建筑提供的资源容量

    /**
     * @brief 构造函数
     */
    BuildingConfigItem(
        std::string name,
        std::string displayName,
        std::string iconPath,
        ResourceType costType,
        int cost,
        int unlockTownHallLevel,
        std::vector<int> maxCountPerTHLevel,
        int capacityIncrease
    ) : name(name), displayName(displayName), iconPath(iconPath), costType(costType),
        cost(cost), unlockTownHallLevel(unlockTownHallLevel),
        maxCountPerTHLevel(maxCountPerTHLevel), capacityIncrease(capacityIncrease) {
    }
};

/**
 * @class GameConfig
 * @brief 游戏配置管理器（单例）
 */
class GameConfig {
public:
    /**
     * @brief 获取单例实例
     * @return GameConfig& 单例引用
     */
    static GameConfig& getInstance();

    /** @brief 初始化配置 */
    void init();

    /**
     * @brief 获取建筑配置
     * @param name 建筑名称
     * @return const BuildingConfigItem* 配置指针
     */
    const BuildingConfigItem* getBuildingConfig(const std::string& name) const;

    /**
     * @brief 获取某大本营等级下建筑的最大允许数量
     * @param name 建筑名称
     * @param townHallLevel 大本营等级
     * @return int 最大数量
     */
    int getMaxBuildingCount(const std::string& name, int townHallLevel) const;

    /** @brief 获取所有商店可售卖的建筑列表 */
    const std::vector<BuildingConfigItem>& getAllBuildings() const;

private:
    GameConfig() = default;
    std::vector<BuildingConfigItem> _buildings;  ///< 建筑配置列表
};
