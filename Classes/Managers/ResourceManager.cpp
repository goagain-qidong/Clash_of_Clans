// 2453619 薛毓哲
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceManager.cpp
 * File Function: 资源管理器实现
 ****************************************************************/
#include "ResourceManager.h"
ResourceManager* ResourceManager::_instance = nullptr;

ResourceManager& ResourceManager::getInstance()
{
    if (!_instance)
    {
        _instance = new ResourceManager();
        _instance->init();
    }
    return *_instance;
}
ResourceManager::ResourceManager() {}

void ResourceManager::init()
{
    // 初始容量 (1级大本营基础容量)
    // 根据需求：金币3000，圣水3000，宝石无上限或很高，工人2个
    _capacities[kGold] = 3000;
    _capacities[kElixir] = 3000;
    _capacities[kGem] = 9999999;
    _capacities[kBuilder] = 2;  // ✅ 修复：初始有2个工人位置
    _capacities[kTroopPopulation] = 0; // 初始没有军营，人口上限为0

    // 初始资源
    _resources[kGold] = 3000;
    _resources[kElixir] = 3000;
    _resources[kGem] = 1000;
    _resources[kBuilder] = 2;  // ✅ 修复：初始拥有2个工人
    _resources[kTroopPopulation] = 0; // 初始没有小兵
}
// 新增：增加容量的方法 (供 BuildingManager 在建造完成后调用)
void ResourceManager::addCapacity(ResourceType type, int amount)
{
    if (amount <= 0) return;
    int currentCap = getResourceCapacity(type);
    setResourceCapacity(type, currentCap + amount);
}
int ResourceManager::getResourceCount(ResourceType type) const
{
    auto it = _resources.find(type);
    return it != _resources.end() ? it->second : 0;
}
int ResourceManager::getResourceCapacity(ResourceType type) const
{
    auto it = _capacities.find(type);
    return it != _capacities.end() ? it->second : 0;
}
void ResourceManager::setResourceCount(ResourceType type, int amount)
{
    int capacity = getResourceCapacity(type);
    // ✅ 修复：调整日志并确保容量检查正确
    if (capacity > 0 && amount > capacity)
    {
        CCLOG("⚠️ 资源 %d 超容！请求：%d，容量：%d，已限制为容量上限", 
              type, amount, capacity);
        amount = capacity;
    }
    if (amount < 0)
    {
        amount = 0;
    }
    _resources[type] = amount;
    if (_onChangeCallback)
    {
        _onChangeCallback(type, _resources[type]);
    }
}
void ResourceManager::setResourceCapacity(ResourceType type, int capacity)
{
    if (capacity < 0)
    {
        capacity = 0;
    }
    _capacities[type] = capacity;
    int current = getResourceCount(type);
    if (current > capacity)
    {
        CCLOG("⚠️ 资源 %d 当前值 %d 超过新容量 %d，已调整为容量上限", 
              type, current, capacity);
        _resources[type] = capacity;
    }
    
    // 容量改变时也触发回调，让HUDLayer更新显示
    if (_onChangeCallback)
    {
        _onChangeCallback(type, current > capacity ? capacity : current);
    }
}
int ResourceManager::addResource(ResourceType type, int amount)
{
    if (amount <= 0)
    {
        return 0;
    }
    int current = getResourceCount(type);
    int capacity = getResourceCapacity(type);
    int actualAdded = amount;
    if (capacity > 0 && current + amount > capacity)
    {
        actualAdded = capacity - current;
    }
    setResourceCount(type, current + actualAdded);
    return actualAdded;
}
bool ResourceManager::hasEnough(ResourceType type, int amount) const
{
    if (amount <= 0)
    {
        return true;
    }
    return getResourceCount(type) >= amount;
}
bool ResourceManager::consume(ResourceType type, int amount)
{
    if (amount <= 0)
    {
        return true;
    }
    if (!hasEnough(type, amount))
    {
        CCLOG("❌ 资源不足！类型：%d，需要：%d，当前：%d", 
              type, amount, getResourceCount(type));
        return false;
    }
    setResourceCount(type, getResourceCount(type) - amount);
    return true;
}
void ResourceManager::setOnResourceChangeCallback(const std::function<void(ResourceType, int)>& callback)
{
    _onChangeCallback = callback;
}

// 人口系统实现
bool ResourceManager::hasTroopSpace(int count) const
{
    if (count <= 0) return true;
    int current = getCurrentTroopCount();
    int capacity = getMaxTroopCapacity();
    return (current + count) <= capacity;
}

bool ResourceManager::addTroops(int count)
{
    if (count <= 0) return false;
    if (!hasTroopSpace(count)) return false;
    
    int current = getCurrentTroopCount();
    setResourceCount(kTroopPopulation, current + count);
    return true;
}
void ResourceManager::fillAllResourcesMax()
{
    CCLOG("\n========================================");
    CCLOG("💰 执行资源全满命令");
    CCLOG("========================================");
    
    // 需要填满的资源类型（不包括人口）
    // 🎯 修复：移除 kTroopPopulation，人口不应该被一键填满
    std::vector<ResourceType> resourceTypes = {
        ResourceType::kGold,
        ResourceType::kElixir,
        ResourceType::kGem,
        ResourceType::kBuilder
    };
    
    // 逐个资源填满到上限
    for (auto type : resourceTypes) {
        int capacity = getResourceCapacity(type);
        
        // 容量为0时跳过
        if (capacity <= 0) {
            CCLOG("  ⊘ 资源类型 %d：容量为0，跳过", (int)type);
            continue;
        }
        
        int oldAmount = getResourceCount(type);
        setResourceCount(type, capacity);
        int newAmount = getResourceCount(type);
        
        CCLOG("  ✓ 资源类型 %d：%d -> %d", (int)type, oldAmount, newAmount);
    }
    
    CCLOG("✅ 资源全满完成！（人口不受影响）");
    CCLOG("========================================\n");
}