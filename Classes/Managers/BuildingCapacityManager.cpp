/****************************************************************
* Project Name:  Clash_of_Clans
* File Name:     BuildingCapacityManager.cpp
* File Function: 建筑增加资源容量类
* Author:        刘相成、薛毓哲
* Update Date:   2025/12/24
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
    if (!building)
    {
        CCLOG("⚠️ Capacity Manager: 传入的建筑指针为空！");
        return;
    }
    
    if (!building->isStorage())
    {
        CCLOG("⚠️ Capacity Manager: %s 不是存储型建筑，跳过", 
              building->getDisplayName().c_str());
        return;
    }

    ResourceType type = building->getResourceType();
    
    // 🔴 修复：确保资源类型正确
    if (type != ResourceType::kGold && type != ResourceType::kElixir)
    {
        CCLOG("⚠️ Capacity Manager: 非法资源类型 %d，建筑: %s，跳过", 
              static_cast<int>(type), building->getDisplayName().c_str());
        return;
    }
    
    // 🔴 新增：根据建筑名称二次验证资源类型
    std::string displayName = building->getDisplayName();
    ResourceType expectedType = type;
    
    if (displayName.find("金币仓库") != std::string::npos || 
        displayName.find("Gold Storage") != std::string::npos)
    {
        expectedType = ResourceType::kGold;
    }
    else if (displayName.find("圣水仓库") != std::string::npos || 
             displayName.find("Elixir Storage") != std::string::npos)
    {
        expectedType = ResourceType::kElixir;
    }
    
    if (type != expectedType)
    {
        CCLOG("❌ Capacity Manager 类型错误！建筑: %s, 报告类型: %s, 期望类型: %s",
              displayName.c_str(),
              type == ResourceType::kGold ? "金币" : "圣水",
              expectedType == ResourceType::kGold ? "金币" : "圣水");
        // 使用根据名称推断的正确类型
        type = expectedType;
    }

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
            CCLOG("➕ Capacity Manager 添加新建筑: %s (类型: %s)", 
                  building->getDisplayName().c_str(),
                  type == ResourceType::kGold ? "金币" : "圣水");
        }
        else
        {
            CCLOG("🔄 Capacity Manager 更新已有建筑: %s (类型: %s)", 
                  building->getDisplayName().c_str(),
                  type == ResourceType::kGold ? "金币" : "圣水");
        }
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

    // 🔴 修复：分别处理金币和圣水，并验证建筑有效性
    // 即使没有任何建筑，也必须保证有基础容量

    // --- 计算金币总容量 ---
    int totalGold = BASE_CAPACITY; // 起点是 3000
    int goldBuildingCount = 0;
    if (_storageBuildings.find(ResourceType::kGold) != _storageBuildings.end())
    {
        // 🔴 修复：使用临时向量存储有效建筑，避免遍历时修改
        std::vector<ResourceBuilding*> validBuildings;
        for (auto* building : _storageBuildings[ResourceType::kGold])
        {
            // 验证建筑有效性
            if (building && building->getReferenceCount() > 0 && !building->isDestroyed())
            {
                // 🔴 修复：二次验证资源类型，防止类型混乱
                if (building->getResourceType() == ResourceType::kGold)
                {
                    int capacity = building->getStorageCapacity();
                    totalGold += capacity;
                    goldBuildingCount++;
                    validBuildings.push_back(building);
                    CCLOG("  📦 金币仓库: %s, 容量: %d", 
                          building->getDisplayName().c_str(), capacity);
                }
                else
                {
                    CCLOG("  ⚠️ 警告：金币列表中发现非金币建筑: %s", 
                          building->getDisplayName().c_str());
                }
            }
        }
        // 更新列表，移除无效指针
        _storageBuildings[ResourceType::kGold] = validBuildings;
    }
    _currentTotalCapacity[ResourceType::kGold] = totalGold;
    resMgr.setResourceCapacity(ResourceType::kGold, totalGold);

    // --- 计算圣水总容量 ---
    int totalElixir = BASE_CAPACITY; // 起点是 3000
    int elixirBuildingCount = 0;
    if (_storageBuildings.find(ResourceType::kElixir) != _storageBuildings.end())
    {
        std::vector<ResourceBuilding*> validBuildings;
        for (auto* building : _storageBuildings[ResourceType::kElixir])
        {
            if (building && building->getReferenceCount() > 0 && !building->isDestroyed())
            {
                if (building->getResourceType() == ResourceType::kElixir)
                {
                    int capacity = building->getStorageCapacity();
                    totalElixir += capacity;
                    elixirBuildingCount++;
                    validBuildings.push_back(building);
                    CCLOG("  📦 圣水仓库: %s, 容量: %d", 
                          building->getDisplayName().c_str(), capacity);
                }
                else
                {
                    CCLOG("  ⚠️ 警告：圣水列表中发现非圣水建筑: %s", 
                          building->getDisplayName().c_str());
                }
            }
        }
        _storageBuildings[ResourceType::kElixir] = validBuildings;
    }
    _currentTotalCapacity[ResourceType::kElixir] = totalElixir;
    resMgr.setResourceCapacity(ResourceType::kElixir, totalElixir);

    CCLOG("📈 容量重算完毕 -> 金币: %d (%d个仓库), 圣水: %d (%d个仓库)", 
          totalGold, goldBuildingCount, totalElixir, elixirBuildingCount);
}

int BuildingCapacityManager::getTotalCapacity(ResourceType type) const
{
    auto it = _currentTotalCapacity.find(type);
    return it != _currentTotalCapacity.end() ? it->second : 0;
}

void BuildingCapacityManager::clearAllBuildings()
{
    _storageBuildings.clear();
    _currentTotalCapacity.clear();
    
    // 重置容量为基础值
    auto& resMgr = ResourceManager::getInstance();
    resMgr.setResourceCapacity(ResourceType::kGold, BASE_CAPACITY);
    resMgr.setResourceCapacity(ResourceType::kElixir, BASE_CAPACITY);
    
    CCLOG("🗑️ BuildingCapacityManager 已清空所有建筑引用");
}