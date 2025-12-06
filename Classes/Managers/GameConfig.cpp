#include "GameConfig.h"
#include <vector> 
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 游戏配置数据
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
GameConfig& GameConfig::getInstance() {
    static GameConfig instance;
    if (instance._buildings.empty()) {
        instance.init();
    }
    return instance;
}

void GameConfig::init() {
    _buildings.clear();
    // ==================== 补回：大本营 ====================
    _buildings.emplace_back(
        "Town Hall",                         // 名字 (需与 ShopLayer 判断逻辑一致)
        "Town Hall",                         // 显示名字
        "buildings/BaseCamp/town-hall-1.png", // 图片路径
        ResourceType::kGold,                 // 资源类型
        0,                                   // 造价 (初始大本营通常免费或已存在)
        1,                                   // 解锁等级
        std::vector<int>{1, 1, 1, 1, 1, 1, 1}, // 所有等级限制只能造 1 个
        1000                                 // 增加资源上限 (可选)
    );

    // 1. 资源建筑
    _buildings.emplace_back(
        "金矿",
        "金矿",
        "buildings/GoldMine/Gold_Mine1.png",
        ResourceType::kElixir,
        150,
        1,
        std::vector<int>{2, 3, 4, 5, 6, 6, 6},
        0
    );

    _buildings.emplace_back(
        "圣水收集器",
        "圣水收集器",
        "buildings/ElixirCollector/Elixir_Collector1.png",
        ResourceType::kGold,
        150,
        1,
        std::vector<int>{2, 3, 4, 5, 6, 6, 6},
        0
    );

    // 2. 存储建筑
    _buildings.emplace_back(
        "金币仓库",
        "金币储罐",
        "buildings/GoldStorage/Gold_Storage1.png",
        ResourceType::kElixir,
        300,
        1,
        std::vector<int>{1, 1, 2, 2, 3, 4, 4},
        1500
    );

    _buildings.emplace_back(
        "圣水仓库",
        "圣水瓶",
        "buildings/ElixirStorage/Elixir_Storage1.png",
        ResourceType::kGold,
        300,
        1,
        std::vector<int>{1, 1, 2, 2, 3, 4, 4},
        1500
    );

    // 3. 军事
    _buildings.emplace_back(
        "兵营",
        "兵营",
        "buildings/Barracks/Barracks1.png",
        ResourceType::kElixir,
        200,
        1,
        std::vector<int>{1, 2, 3, 4, 4, 4, 4},
        0
    );

    _buildings.emplace_back(
        "军营",
        "兵力集结地",
        "buildings/ArmyCamp/Army_Camp1.png",
        ResourceType::kElixir,
        250,
        1,
        std::vector<int>{1, 1, 2, 3, 4, 4, 4},
        0
    );

    // 4. 防御
    _buildings.emplace_back(
        "炮塔",
        "加农炮",
        "buildings/Cannon_Static/Cannon1.png",
        ResourceType::kGold,
        250,
        1,
        std::vector<int>{2, 2, 2, 3, 4, 5, 5},
        0
    );

    _buildings.emplace_back(
        "箭塔",
        "箭塔",
        "buildings/ArcherTower/Archer_Tower1.png",
        ResourceType::kGold,
        1000,
        2,
        std::vector<int>{0, 1, 2, 3, 4, 5, 6},
        0
    );

    _buildings.emplace_back(
        "城墙",
        "城墙",
        "buildings/Wall/Wall1.png",
        ResourceType::kGold,
        50,
        1,
        std::vector<int>{25, 50, 75, 100, 150, 200, 250},
        0
    );

    // 5. 建筑工人
    _buildings.emplace_back(
        "建筑工人小屋",
        "建筑工人小屋",
        "buildings/BuildersHut/Builders_Hut1.png",
        ResourceType::kGem,
        500,
        1,
        std::vector<int>{5, 5, 5, 5, 5, 5, 5},
        0
    );
}

const BuildingConfigItem* GameConfig::getBuildingConfig(const std::string& name) const {
    for (const auto& item : _buildings) {
        if (item.name == name) return &item;
    }
    return nullptr;
}

int GameConfig::getMaxBuildingCount(const std::string& name, int townHallLevel) const {
    const auto* cfg = getBuildingConfig(name);
    if (!cfg) return 0;

    int index = townHallLevel - 1;
    if (index < 0) return 0;
    if (index >= cfg->maxCountPerTHLevel.size()) {
        return cfg->maxCountPerTHLevel.back(); // 超过配置等级则取最大值
    }
    return cfg->maxCountPerTHLevel[index];
}

const std::vector<BuildingConfigItem>& GameConfig::getAllBuildings() const {
    return _buildings;
}