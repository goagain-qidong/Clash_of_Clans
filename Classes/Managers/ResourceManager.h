// 2453619 薛毓哲
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceManager.h
 * File Function: 资源管理器
 ****************************************************************/
#pragma once
#include "cocos2d.h"
#include <functional>
#include <map>
#include <string>
// 使用非作用域枚举，确保可在 switch-case 中作为常量使用
// 唯一定义位置，其他头文件通过 #include "ResourceManager.h" 引用
enum ResourceType
{
    kGold = 0,
    kElixir = 1,
    kGem = 2,
    kBuilder = 3
};
class ResourceManager
{
public:
    // 单例访问方法（双命名，兼容现有调用）
    static ResourceManager* GetInstance();
    static ResourceManager& getInstance();
    // 初始化资源
    void Init();
    // 资源读方法
    int GetResourceCount(ResourceType type) const;
    int GetResourceCapacity(ResourceType type) const;
    // 资源写方法
    void SetResourceCount(ResourceType type, int amount);
    void SetResourceCapacity(ResourceType type, int capacity);
    // 资源增加/消耗与校验（大写命名）
    int AddResource(ResourceType type, int amount);
    bool HasEnough(ResourceType type, int amount) const;
    bool ConsumeResource(ResourceType type, int amount);
    // 资源增加/消耗与校验（小写命名，兼容 BaseBuilding 调用）
    int addResource(ResourceType type, int amount) { return AddResource(type, amount); }
    bool hasEnough(ResourceType type, int amount) const { return HasEnough(type, amount); }
    bool consume(ResourceType type, int amount) { return ConsumeResource(type, amount); }
    // 设置资源变化回调
    void SetOnResourceChangeCallback(const std::function<void(ResourceType, int)>& callback);
    void AddCapacity(ResourceType type, int amount);
private:
    ResourceManager();
    static ResourceManager* _instance;
    std::map<ResourceType, int> _resources;
    std::map<ResourceType, int> _capacities;
    std::function<void(ResourceType, int)> _onChangeCallback;
};