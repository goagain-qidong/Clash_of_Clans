/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingCapacityManager.h
 * File Function: 建筑增加资源容量类
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include "Buildings/ResourceBuilding.h"

#include <vector>
#include <map>

/**
 * @class BuildingCapacityManager
 * @brief 建筑容量管理器（单例）
 *
 * 职责：
 * - 跟踪所有存储型建筑
 * - 实时计算所有存储建筑提供的总容量
 * - 更新ResourceManager中的容量上限
 */
class BuildingCapacityManager : public cocos2d::Node
{
public:
    /**
     * @brief 获取单例实例
     * @return BuildingCapacityManager& 单例引用
     */
    static BuildingCapacityManager& getInstance();

    virtual bool init() override;

    /**
     * @brief 注册或取消注册建筑
     * @param building 存储型建筑
     * @param added 是否是添加
     */
    void registerOrUpdateBuilding(ResourceBuilding* building, bool added);

    /** @brief 重新计算并设置容量上限 */
    void recalculateCapacity();

    /**
     * @brief 获取某资源类型当前的总容量
     * @param type 资源类型
     * @return int 总容量
     */
    int getTotalCapacity(ResourceType type) const;

private:
    BuildingCapacityManager();

    std::map<ResourceType, std::vector<ResourceBuilding*>> _storageBuildings;  ///< 存储建筑列表
    std::map<ResourceType, int> _currentTotalCapacity;  ///< 当前容量总和
};
