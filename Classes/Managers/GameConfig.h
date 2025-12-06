#pragma once
/****************************************************************
  * Project Name:  Clash_of_Clans
  * File Name:     WallBuilding.cpp
  * File Function: 游戏配置数据
  * Author:        刘相成
  * Update Date:   2025/12/06
  * License:       MIT License
  ****************************************************************/
#include <string>
#include <map>
#include <vector>
#include "ResourceManager.h" // 引用 ResourceType

struct BuildingConfigItem {
    std::string name;           // 建筑名称 (key)
    std::string displayName;    // 显示名称
    std::string iconPath;       // 商店图标路径
    ResourceType costType;      // 花费类型
    int cost;                   // 造价 (1级)
    int unlockTownHallLevel;    // 解锁需要的大本营等级
    // 不同大本营等级对应的最大建筑数量 (索引0=TH1, 索引1=TH2...)
    std::vector<int> maxCountPerTHLevel;
    // 该建筑提供的资源容量 (如果是仓库)
    int capacityIncrease;
    // === 新增：显式构造函数 ===
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

class GameConfig {
public:
    static GameConfig& getInstance();

    // 初始化配置
    void init();

    // 获取建筑配置
    const BuildingConfigItem* getBuildingConfig(const std::string& name) const;

    // 获取某大本营等级下，某建筑的最大允许数量
    int getMaxBuildingCount(const std::string& name, int townHallLevel) const;

    // 获取所有商店可售卖的建筑列表
    const std::vector<BuildingConfigItem>& getAllBuildings() const;

private:
    GameConfig() = default;
    std::vector<BuildingConfigItem> _buildings;
};
