/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 建筑增加资源容量类
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
#include "BuildingCapacityManager.h"
#include "Managers/ResourceManager.h"

USING_NS_CC;

BuildingCapacityManager& BuildingCapacityManager::getInstance()
{
    static BuildingCapacityManager instance;
    return instance;
}

BuildingCapacityManager::BuildingCapacityManager()
{
}

bool BuildingCapacityManager::init()
{
    if (!Node::init()) return false;
    // 确保 Capacity Manager 是场景的一部分，以便能够使用 schedule/回调等
    CCLOG("✅ BuildingCapacityManager initialized.");
    return true;
}

void BuildingCapacityManager::registerOrUpdateBuilding(ResourceBuilding* building, bool added)
{
    if (!building || !building->isStorage()) return;

    ResourceType type = building->getResourceType();

    if (added)
    {
        // 检查是否已存在，避免重复添加
        bool found = false;
        for (auto* b : _storageBuildings[type])
        {
            if (b == building)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            _storageBuildings[type].push_back(building);
        }
        // 如果已存在，它只是更新了等级，容量计算逻辑在 recalculateCapacity 中处理。

        CCLOG("🔄 Capacity Manager 注册/更新建筑: %s", building->getDisplayName().c_str());
    }
    else // 移除建筑
    {
        auto& list = _storageBuildings[type];
        list.erase(std::remove(list.begin(), list.end(), building), list.end());

        CCLOG("🗑️ Capacity Manager 移除建筑: %s", building->getDisplayName().c_str());
    }

    // 无论添加、更新还是移除，都需要重新计算总容量
    recalculateCapacity();
}

// 🔴 1. 定义基础容量常量 (如果头文件没定义，这里定义)
static const int BASE_CAPACITY = 3000;

void BuildingCapacityManager::recalculateCapacity()
{
    // 清空缓存
    _currentTotalCapacity.clear();
    auto& resMgr = ResourceManager::getInstance();

    // 🔴 2. 分别处理金币和圣水
    // 即使没有任何建筑，也必须保证有基础容量

    // --- 计算金币总容量 ---
    int totalGold = BASE_CAPACITY; // 起点是 3000
    if (_storageBuildings.find(ResourceType::kGold) != _storageBuildings.end())
    {
        for (auto* building : _storageBuildings[ResourceType::kGold])
        {
            if (building) {
                // 累加每个仓库提供的容量
                totalGold += building->getStorageCapacity();
            }
        }
    }
    _currentTotalCapacity[ResourceType::kGold] = totalGold;
    resMgr.setResourceCapacity(ResourceType::kGold, totalGold);

    // --- 计算圣水总容量 ---
    int totalElixir = BASE_CAPACITY; // 起点是 3000
    if (_storageBuildings.find(ResourceType::kElixir) != _storageBuildings.end())
    {
        for (auto* building : _storageBuildings[ResourceType::kElixir])
        {
            if (building) {
                // 累加每个仓库提供的容量
                totalElixir += building->getStorageCapacity();
            }
        }
    }
    _currentTotalCapacity[ResourceType::kElixir] = totalElixir;
    resMgr.setResourceCapacity(ResourceType::kElixir, totalElixir);

    CCLOG("📈 容量重算完毕 -> 金币: %d, 圣水: %d", totalGold, totalElixir);
}

int BuildingCapacityManager::getTotalCapacity(ResourceType type) const
{
    auto it = _currentTotalCapacity.find(type);
    return it != _currentTotalCapacity.end() ? it->second : 0;
}