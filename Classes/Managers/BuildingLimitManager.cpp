/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingLimitManager.cpp
 * File Function: 建筑数量上限管理器实现
 * Author:        刘相成
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#include "BuildingLimitManager.h"
#include "cocos2d.h"

USING_NS_CC;

BuildingLimitManager* BuildingLimitManager::getInstance()
{
    static BuildingLimitManager instance;
    return &instance;
}

BuildingLimitManager::BuildingLimitManager()
{
    reset();
}

bool BuildingLimitManager::init()
{
    if (!Node::init()) return false;
    CCLOG("✅ BuildingLimitManager initialized");
    return true;
}

void BuildingLimitManager::reset()
{
    _limits.clear();
    _buildingCounts.clear();
    _cachedTownHallLevel = 1;
    
    // 初始化各建筑的上限与计数
    _limits["BuildersHut"] = -1;           // 无限制
    _limits["Wall"] = 50;                  // 初始上限50（TH Lv.1）
    _limits["Cannon"] = 1;                 // 初始上限1
    _limits["ArcherTower"] = 1;            // 初始上限1
    _limits["WizardTower"] = 1;            // 初始上限1
    _limits["GoldMine"] = 1;               // 初始上限1
    _limits["ElixirCollector"] = 1;        // 初始上限1
    _limits["GoldStorage"] = 1;            // 初始上限1
    _limits["ElixirStorage"] = 1;          // 初始上限1
    _limits["Barracks"] = 1;               // 初始上限1
    _limits["ArmyCamp"] = 1;               // 初始上限1
    
    // 所有计数初始化为 0
    for (auto& pair : _limits) {
        _buildingCounts[pair.first] = 0;
    }
    
    CCLOG("✅ BuildingLimitManager reset - TH Lv.1");
}

int BuildingLimitManager::getLimit(const std::string& buildingName) const
{
    auto it = _limits.find(buildingName);
    if (it != _limits.end()) {
        return it->second;
    }
    // 未找到该建筑，默认返回大本营等级的上限
    return std::max(1, _cachedTownHallLevel);
}

void BuildingLimitManager::addLimit(const std::string& buildingName, int increment)
{
    if (_limits.find(buildingName) == _limits.end()) {
        _limits[buildingName] = std::max(1, _cachedTownHallLevel) + increment;
    } else {
        int current = _limits[buildingName];
        if (current == -1) {
            // 无限制，保持无限制
            return;
        }
        _limits[buildingName] = current + increment;
    }
    
    CCLOG("🔧 %s limit increased by %d, new limit: %d", 
          buildingName.c_str(), increment, _limits[buildingName]);
}

void BuildingLimitManager::setLimit(const std::string& buildingName, int newLimit)
{
    _limits[buildingName] = newLimit;
    if (newLimit == -1) {
        CCLOG("🔧 %s set to unlimited", buildingName.c_str());
    } else {
        CCLOG("🔧 %s limit set to: %d", buildingName.c_str(), newLimit);
    }
}

void BuildingLimitManager::updateLimitsFromTownHall(int townHallLevel)
{
    int prevLevel = _cachedTownHallLevel;
    _cachedTownHallLevel = townHallLevel;
    
    CCLOG("📊 Updating building limits: TownHall Lv.%d -> Lv.%d", prevLevel, townHallLevel);
    
    // ========== 规则1：BuildersHut 保持无限制 ==========
    _limits["BuildersHut"] = -1;
    CCLOG("  ✓ BuildersHut: unlimited");
    
    // ========== 规则2：Wall 每升级增加50 ==========
    // 初始值：50（TH Lv.1）
    // Lv.2: 50 + 50 = 100
    // Lv.3: 100 + 50 = 150
    // ...
    int currentWallLimit = _limits["Wall"];
    if (currentWallLimit == -1) {
        currentWallLimit = 50; // 防守初始化错误
    }
    int newWallLimit = currentWallLimit + (townHallLevel - prevLevel) * 50;
    _limits["Wall"] = newWallLimit;
    CCLOG("  ✓ Wall: %d -> %d (增加 %d)", 
          currentWallLimit, newWallLimit, (townHallLevel - prevLevel) * 50);
    
    // ========== 规则3：其他建筑 ==========
    // 每升级增加1，且不少于大本营等级
    std::vector<std::string> otherBuildings = {
        "Cannon", "ArcherTower", "WizardTower",
        "GoldMine", "ElixirCollector", "GoldStorage", "ElixirStorage",
        "Barracks", "ArmyCamp"
    };
    
    for (const auto& building : otherBuildings) {
        int currentLimit = _limits[building];
        // 每级增加1
        int newLimit = currentLimit + (townHallLevel - prevLevel);
        // 确保不小于大本营等级
        newLimit = std::max(newLimit, townHallLevel);
        
        _limits[building] = newLimit;
        CCLOG("  ✓ %s: %d -> %d", building.c_str(), currentLimit, newLimit);
    }
    
    CCLOG("📊 Building limits update complete");
}

int BuildingLimitManager::getBuildingCount(const std::string& buildingName) const
{
    auto it = _buildingCounts.find(buildingName);
    if (it != _buildingCounts.end()) {
        return it->second;
    }
    return 0;
}

void BuildingLimitManager::recordBuilding(const std::string& buildingName)
{
    if (_buildingCounts.find(buildingName) == _buildingCounts.end()) {
        _buildingCounts[buildingName] = 1;
    } else {
        _buildingCounts[buildingName]++;
    }
    
    CCLOG("📈 Building recorded: %s (count: %d / limit: %d)", 
          buildingName.c_str(), 
          _buildingCounts[buildingName],
          getLimit(buildingName));
}

void BuildingLimitManager::removeBuilding(const std::string& buildingName)
{
    if (_buildingCounts.find(buildingName) != _buildingCounts.end()) {
        _buildingCounts[buildingName]--;
        if (_buildingCounts[buildingName] < 0) {
            _buildingCounts[buildingName] = 0;
        }
    }
    
    CCLOG("📉 Building removed: %s (count: %d / limit: %d)", 
          buildingName.c_str(), 
          _buildingCounts[buildingName],
          getLimit(buildingName));
}

bool BuildingLimitManager::canBuild(const std::string& buildingName) const
{
    int limit = getLimit(buildingName);
    
    // 无限制
    if (limit == -1) {
        return true;
    }
    
    int currentCount = getBuildingCount(buildingName);
    bool canBuild = currentCount < limit;
    
    if (!canBuild) {
        CCLOG("❌ Cannot build %s: count (%d) >= limit (%d)", 
              buildingName.c_str(), currentCount, limit);
    }
    
    return canBuild;
}