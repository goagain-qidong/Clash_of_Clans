#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 建筑增加资源容量类
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
#include "cocos2d.h"
#include <vector>
#include <map>
#include "Buildings/ResourceBuilding.h"

/**
 * @class BuildingCapacityManager
 * @brief 建筑容量管理器（单例）
 * * 职责：
 * - 跟踪所有存储型建筑 (ResourceBuilding::isStorage())
 * - 实时计算所有存储建筑提供的总容量
 * - 更新 ResourceManager 中的容量上限
 */
class BuildingCapacityManager : public cocos2d::Node
{
public:
    static BuildingCapacityManager& getInstance();

    virtual bool init() override;

    /**
     * @brief 注册或取消注册建筑，并触发容量重新计算
     * @param building 存储型建筑
     * @param added 是否是添加（true=建造/升级，false=移除）
     */
    void registerOrUpdateBuilding(ResourceBuilding* building, bool added);

    /**
     * @brief 重新计算并设置 ResourceManager 的容量上限
     */
    void recalculateCapacity();

    /**
     * @brief 获取某资源类型当前的总容量
     * @param type 资源类型
     */
    int getTotalCapacity(ResourceType type) const;

private:
    BuildingCapacityManager();

    // 存储类型 -> 关联的资源建筑列表
    std::map<ResourceType, std::vector<ResourceBuilding*>> _storageBuildings;

    // 当前计算出的容量总和
    std::map<ResourceType, int> _currentTotalCapacity;
};
