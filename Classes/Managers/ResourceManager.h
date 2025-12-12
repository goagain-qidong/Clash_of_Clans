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
    kBuilder = 3,
    kTroopPopulation = 4  // 小兵人口
};
class ResourceManager
{
public:
    // 单例访问方法
    static ResourceManager& getInstance();
    
    // 初始化资源
    void init();
    
    // 资源读方法
    int getResourceCount(ResourceType type) const;
    int getResourceCapacity(ResourceType type) const;
    
    // 资源写方法
    void setResourceCount(ResourceType type, int amount);
    void setResourceCapacity(ResourceType type, int capacity);
    
    // 资源增加/消耗与校验
    int addResource(ResourceType type, int amount);
    bool hasEnough(ResourceType type, int amount) const;
    bool consume(ResourceType type, int amount);
    
    // 设置资源变化回调
    void setOnResourceChangeCallback(const std::function<void(ResourceType, int)>& callback);
    void addCapacity(ResourceType type, int amount);
    
    // 人口系统专用接口
    int getCurrentTroopCount() const { return getResourceCount(kTroopPopulation); }
    int getMaxTroopCapacity() const { return getResourceCapacity(kTroopPopulation); }
    bool hasTroopSpace(int count) const;
    bool addTroops(int count);
    
    /**
     * @brief 将所有资源填满到容量上限
     */
    void fillAllResourcesMax();
private:
    ResourceManager();
    static ResourceManager* _instance;
    std::map<ResourceType, int> _resources;
    std::map<ResourceType, int> _capacities;
    std::function<void(ResourceType, int)> _onChangeCallback;
};