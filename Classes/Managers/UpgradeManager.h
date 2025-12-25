/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UpgradeManager.h
 * File Function: 建筑升级管理器（处理升级队列、工人分配、倒计时）
 * Author:        薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "cocos2d.h"
#include <vector>
#include <functional>
#include <string>

// 前向声明
class BaseBuilding;

/**
 * @struct UpgradeTaskData
 * @brief 升级任务序列化数据（用于保存/加载）
 */
struct UpgradeTaskData
{
    std::string buildingName;    ///< 建筑名称（用于匹配）
    float gridX;                 ///< 建筑网格X坐标
    float gridY;                 ///< 建筑网格Y坐标
    float totalTime;             ///< 总升级时间（秒）
    float elapsedTime;           ///< 已经过的时间（秒）
    int cost;                    ///< 升级费用
    bool useBuilder;             ///< 是否占用工人
    
    UpgradeTaskData()
        : gridX(0), gridY(0), totalTime(0), elapsedTime(0), cost(0), useBuilder(true) {}
};

/**
 * @struct UpgradeTask
 * @brief 升级任务结构体
 */
struct UpgradeTask
{
    BaseBuilding* building;      // 正在升级的建筑
    float totalTime;             // 总升级时间（秒）
    float elapsedTime;           // 已经过的时间（秒）
    int cost;                    // 升级费用
    bool useBuilder;             // 是否占用工人
    
    UpgradeTask(BaseBuilding* bld, float time, int costValue, bool needBuilder = true)
        : building(bld), totalTime(time), elapsedTime(0.0f), cost(costValue), useBuilder(needBuilder)
    {
    }
    
    /** @brief 获取升级进度（0.0 ~ 1.0） */
    float getProgress() const
    {
        if (totalTime <= 0.0f) return 1.0f;
        return std::min(elapsedTime / totalTime, 1.0f);
    }
    
    /** @brief 获取剩余时间（秒） */
    float getRemainingTime() const
    {
        return std::max(0.0f, totalTime - elapsedTime);
    }
};

/**
 * @class UpgradeManager
 * @brief 建筑升级管理器（单例）
 * 
 * 功能：
 * - 管理全局升级队列
 * - 分配建筑工人
 * - 处理升级倒计时
 * - 支持作弊模式（跳过时间/工人限制）
 */
class UpgradeManager : public cocos2d::Node
{
public:
static UpgradeManager* getInstance();
    
// 销毁单例实例（防止内存泄漏）
static void destroyInstance();
    
virtual bool init() override;
    
    // ==================== 核心接口 ====================
    
    /**
     * @brief 检查是否可以开始升级（不扣资源，只检查工人和队列）
     * @param building 要升级的建筑
     * @param needBuilder 是否需要工人
     * @return 是否可以开始升级
     */
    bool canStartUpgrade(BaseBuilding* building, bool needBuilder = true);
    
    /**
     * @brief 开始升级建筑
     * @param building 要升级的建筑
     * @param cost 升级费用
     * @param time 升级时间（秒）
     * @param needBuilder 是否需要工人（默认需要）
     * @return 是否成功开始升级
     */
    bool startUpgrade(BaseBuilding* building, int cost, float time, bool needBuilder = true);
    
    /**
     * @brief 取消升级（退还部分资源和工人）
     * @param building 要取消升级的建筑
     * @return 是否成功取消
     */
    bool cancelUpgrade(BaseBuilding* building);
    
    /**
     * @brief 立即完成升级（宝石加速或作弊模式）
     * @param building 要完成的建筑
     * @return 是否成功完成
     */
    bool finishUpgradeNow(BaseBuilding* building);
    
    /**
     * @brief 检查建筑是否正在升级
     */
    bool isUpgrading(BaseBuilding* building) const;
    
    /**
     * @brief 获取建筑的升级任务
     */
    UpgradeTask* getUpgradeTask(BaseBuilding* building) const;
    
    /**
     * @brief 获取当前升级队列长度
     */
    int getUpgradeQueueLength() const { return static_cast<int>(_upgradeTasks.size()); }
    
    /**
     * @brief 获取空闲工人数量
     */
    int getAvailableBuilders() const;
    
    /**
     * @brief 清理所有升级任务（场景切换时调用，防止野指针）
     */
    void clearAllUpgradeTasks();
    
    // ==================== 序列化接口 ====================
    
    /**
     * @brief 序列化所有升级任务（保存状态）
     * @return 升级任务数据列表
     */
    std::vector<UpgradeTaskData> serializeUpgradeTasks() const;
    
    /**
     * @brief 恢复升级任务（加载状态）
     * @param tasksData 升级任务数据列表
     * @param buildings 当前场景中的建筑列表
     */
    void restoreUpgradeTasks(const std::vector<UpgradeTaskData>& tasksData,
                             const cocos2d::Vector<BaseBuilding*>& buildings);
    
    // ==================== 回调接口 ====================
    
    /**
     * @brief 设置工人数量变化回调（用于UI更新）
     * @param callback 回调函数，参数为当前可用工人数
     */
    void setOnAvailableBuilderChanged(const std::function<void(int)>& callback)
    {
        _onAvailableBuildersChanged = callback;
    }
    
    // ==================== 每帧更新 ====================
    void update(float dt);
    
    // ==================== 🎮 作弊模式接口 ====================
    
    /**
     * @brief 设置作弊模式（跳过时间和工人限制）
     * @param enabled 是否启用
     */
    void setCheatMode(bool enabled) { _cheatModeEnabled = enabled; }
    
    /**
     * @brief 检查是否启用作弊模式
     */
    bool isCheatModeEnabled() const { return _cheatModeEnabled; }

private:
UpgradeManager();
~UpgradeManager();
static UpgradeManager* _instance;
    
    std::vector<UpgradeTask> _upgradeTasks;  // 升级任务列表
    bool _cheatModeEnabled = false;          // 作弊模式开关
    std::function<void(int)> _onAvailableBuildersChanged;  // ✅ 工人数量变化回调
    
    /**
     * @brief 完成升级任务
     */
    void completeUpgrade(UpgradeTask& task);
    
    /**
     * @brief 分配工人
     */
    bool allocateBuilder();
    
    /**
     * @brief 释放工人
     */
    void releaseBuilder();
};
