/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingConfig.h
 * File Function: 建筑配置数据管理类，负责加载和提供建筑的各等级配置数据                            
 * Author:        赵崇治
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "cocos2d.h"
#include "Buildings/BaseBuilding.h" // 引用 BuildingType 枚举

struct BuildingLevelData {
    int level;
    int hp;
    int upgradeCost;
    int upgradeTime; // 秒
    std::string imagePath;
    // 可以扩展更多字段，如 capacity, productionRate 等
};

class BuildingConfig {
public:
    static BuildingConfig* getInstance();
    
    // 获取指定建筑、指定等级的配置数据
    const BuildingLevelData* getData(BuildingType type, int level);

    // 获取最大等级
    int getMaxLevel(BuildingType type);

private:
    BuildingConfig();
    void initData();
    
    // 使用 map 存储不同类型建筑的配置：Map<BuildingType, Vector<LevelData>>
    std::map<BuildingType, std::vector<BuildingLevelData>> _configs;
};