// 2453619 薛毓哲
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceManager.cpp
 * File Function: 资源管理器实现
 ****************************************************************/
#include "ResourceManager.h"
ResourceManager* ResourceManager::_instance = nullptr;
ResourceManager* ResourceManager::GetInstance()
{
    if (!_instance)
    {
        _instance = new ResourceManager();
        _instance->Init();
    }
    return _instance;
}
ResourceManager& ResourceManager::getInstance()
{
    return *GetInstance();
}
ResourceManager::ResourceManager() {}
void ResourceManager::Init()
{
    // 初始容量 (1级大本营基础容量)
    // 根据需求：金币3000，圣水3000，宝石无上限或很高，工人2个
    _capacities[kGold] = 3000;
    _capacities[kElixir] = 3000;
    _capacities[kGem] = 9999999;
    _capacities[kBuilder] = 5; // 总上限5个

    // 初始资源
    _resources[kGold] = 3000;
    _resources[kElixir] = 3000;
    _resources[kGem] = 1000;
    _resources[kBuilder] = 2; // 初始2个工人
}
// 新增：增加容量的方法 (供 BuildingManager 在建造完成后调用)
void ResourceManager::AddCapacity(ResourceType type, int amount)
{
    if (amount <= 0) return;
    int currentCap = GetResourceCapacity(type);
    SetResourceCapacity(type, currentCap + amount);
}
int ResourceManager::GetResourceCount(ResourceType type) const
{
    auto it = _resources.find(type);
    return it != _resources.end() ? it->second : 0;
}
int ResourceManager::GetResourceCapacity(ResourceType type) const
{
    auto it = _capacities.find(type);
    return it != _capacities.end() ? it->second : 0;
}
void ResourceManager::SetResourceCount(ResourceType type, int amount)
{
    int capacity = GetResourceCapacity(type);
    if (capacity > 0 && amount > capacity)
    {
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
void ResourceManager::SetResourceCapacity(ResourceType type, int capacity)
{
    if (capacity < 0)
    {
        capacity = 0;
    }
    _capacities[type] = capacity;
    int current = GetResourceCount(type);
    if (current > capacity)
    {
        _resources[type] = capacity;
        if (_onChangeCallback)
        {
            _onChangeCallback(type, _resources[type]);
        }
    }
}
int ResourceManager::AddResource(ResourceType type, int amount)
{
    if (amount <= 0)
    {
        return 0;
    }
    int current = GetResourceCount(type);
    int capacity = GetResourceCapacity(type);
    int actualAdded = amount;
    if (current + amount > capacity)
    {
        actualAdded = capacity - current;
    }
    SetResourceCount(type, current + actualAdded);
    return actualAdded;
}
bool ResourceManager::HasEnough(ResourceType type, int amount) const
{
    if (amount <= 0)
    {
        return true;
    }
    return GetResourceCount(type) >= amount;
}
bool ResourceManager::ConsumeResource(ResourceType type, int amount)
{
    if (amount <= 0)
    {
        return true;
    }
    if (!HasEnough(type, amount))
    {
        return false;
    }
    SetResourceCount(type, GetResourceCount(type) - amount);
    return true;
}
void ResourceManager::SetOnResourceChangeCallback(const std::function<void(ResourceType, int)>& callback)
{
    _onChangeCallback = callback;
}