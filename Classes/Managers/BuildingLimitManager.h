/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingLimitManager.h
 * File Function: 建筑数量上限管理器
 * Author:        刘相成
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <map>
#include <string>
#include "cocos2d.h"

/**
 * @class BuildingLimitManager
 * @brief 管理不同建筑类型的数量限制（单例）
 * 
 * 规则：
 * - BuildersHut（建筑工人小屋）：无限制
 * - Wall（城墙）：初始上限10，大本营每升一级增加50
 * - 其他建筑：上限 = max(1, 大本营等级)
 */
class BuildingLimitManager : public cocos2d::Node
{
public:
    static BuildingLimitManager* getInstance();
    
    virtual bool init() override;
    
    /**
     * @brief 获取某建筑类型的当前上限
     * @param buildingName 建筑名称（如 "Wall", "Cannon", "BuildersHut" 等）
     * @return 该建筑类型允许建造的最大数量，-1 表示无限制
     */
    int getLimit(const std::string& buildingName) const;
    
    /**
     * @brief 增加某建筑类型的上限
     * @param buildingName 建筑名称
     * @param increment 增加数量
     */
    void addLimit(const std::string& buildingName, int increment);
    
    /**
     * @brief 设置某建筑的上限
     * @param buildingName 建筑名称
     * @param newLimit 新的上限数（-1 表示无限制）
     */
    void setLimit(const std::string& buildingName, int newLimit);
    
    /**
     * @brief 根据大本营等级重新计算所有建筑的上限
     * @param townHallLevel 大本营等级
     */
    void updateLimitsFromTownHall(int townHallLevel);
    
    /**
     * @brief 重置所有限制（场景初始化时调用）
     */
    void reset();
    
    /**
     * @brief 获取某建筑的当前数量
     * @param buildingName 建筑名称
     * @return 该建筑当前建造的数量
     */
    int getBuildingCount(const std::string& buildingName) const;
    
    /**
     * @brief 记录建筑建造（数量+1）
     * @param buildingName 建筑名称
     */
    void recordBuilding(const std::string& buildingName);
    
    /**
     * @brief 记录建筑拆除（数量-1）
     * @param buildingName 建筑名称
     */
    void removeBuilding(const std::string& buildingName);
    
    /**
     * @brief 检查是否可以建造该建筑
     * @param buildingName 建筑名称
     * @return true 如果还有建造空间
     */
    bool canBuild(const std::string& buildingName) const;

private:
    BuildingLimitManager();
    
    // 上限表：建筑名 -> 最大数量（-1 表示无限制）
    std::map<std::string, int> _limits;
    
    // 计数表：建筑名 -> 当前数量
    std::map<std::string, int> _buildingCounts;
    
    // 大本营等级缓存
    int _cachedTownHallLevel = 1;
};