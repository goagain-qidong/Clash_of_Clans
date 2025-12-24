/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceManager.h
 * File Function: 资源管理器
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"

#include <functional>
#include <map>
#include <string>

/**
 * @enum ResourceType
 * @brief 资源类型枚举
 */
enum ResourceType
{
    kGold = 0,            ///< 金币
    kElixir = 1,          ///< 圣水
    kGem = 2,             ///< 宝石
    kBuilder = 3,         ///< 建筑工人
    kTroopPopulation = 4  ///< 小兵人口
};

/**
 * @class ResourceManager
 * @brief 资源管理器（单例）
 */
class ResourceManager
{
public:
    /**
     * @brief 获取单例实例
     * @return ResourceManager& 单例引用
     */
    static ResourceManager& getInstance();

    /** @brief 销毁单例实例 */
    static void destroyInstance();

    /** @brief 初始化资源 */
    void init();

    /**
     * @brief 获取资源数量
     * @param type 资源类型
     * @return int 资源数量
     */
    int getResourceCount(ResourceType type) const;

    /**
     * @brief 获取资源容量
     * @param type 资源类型
     * @return int 资源容量
     */
    int getResourceCapacity(ResourceType type) const;

    /**
     * @brief 设置资源数量
     * @param type 资源类型
     * @param amount 数量
     */
    void setResourceCount(ResourceType type, int amount);

    /**
     * @brief 设置资源容量
     * @param type 资源类型
     * @param capacity 容量
     */
    void setResourceCapacity(ResourceType type, int capacity);

    /**
     * @brief 增加资源
     * @param type 资源类型
     * @param amount 数量
     * @return int 实际增加的数量
     */
    int addResource(ResourceType type, int amount);

    /**
     * @brief 检查资源是否足够
     * @param type 资源类型
     * @param amount 数量
     * @return bool 是否足够
     */
    bool hasEnough(ResourceType type, int amount) const;

    /**
     * @brief 消耗资源
     * @param type 资源类型
     * @param amount 数量
     * @return bool 是否成功
     */
    bool consume(ResourceType type, int amount);

    /**
     * @brief 设置资源变化回调
     * @param callback 回调函数
     */
    void setOnResourceChangeCallback(const std::function<void(ResourceType, int)>& callback);

    /**
     * @brief 增加资源容量
     * @param type 资源类型
     * @param amount 数量
     */
    void addCapacity(ResourceType type, int amount);

    /** @brief 获取当前部队人口 */
    int getCurrentTroopCount() const { return getResourceCount(kTroopPopulation); }

    /** @brief 获取最大部队容量 */
    int getMaxTroopCapacity() const { return getResourceCapacity(kTroopPopulation); }

    /**
     * @brief 检查是否有足够的部队空间
     * @param count 需要的空间
     * @return bool 是否有空间
     */
    bool hasTroopSpace(int count) const;

    /**
     * @brief 添加部队
     * @param count 数量
     * @return bool 是否成功
     */
    bool addTroops(int count);

    /** @brief 将所有资源填满到容量上限 */
    void fillAllResourcesMax();

private:
    ResourceManager();
    ~ResourceManager();

    static ResourceManager* _instance;                    ///< 单例实例
    std::map<ResourceType, int> _resources;               ///< 资源数量
    std::map<ResourceType, int> _capacities;              ///< 资源容量
    std::function<void(ResourceType, int)> _onChangeCallback;  ///< 变化回调
};