/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyBuilding.h
 * File Function: 军事建筑类（兵营、训练营等）
 * Author:        薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#ifndef ARMY_BUILDING_H_
#define ARMY_BUILDING_H_

#include "BaseBuilding.h"
#include "Unit/UnitTypes.h"

#include <functional>
#include <queue>

class BaseUnit;

/**
 * @struct TrainingTask
 * @brief 训练任务结构体
 */
struct TrainingTask
{
    UnitType unitType;     ///< 兵种类型
    float trainingTime;    ///< 训练时间（秒）
    float elapsedTime;     ///< 已经过的时间
    int cost;              ///< 训练费用

    TrainingTask(UnitType type, float time, int costValue)
        : unitType(type), trainingTime(time), elapsedTime(0.0f), cost(costValue)
    {}
};

/**
 * @class ArmyBuilding
 * @brief 军事建筑类 - 用于训练士兵
 * @note 使用数据驱动架构，基础配置数据由 BaseBuilding::getStaticConfig() 管理
 *       训练系统相关的特有逻辑保留在子类中
 */
class ArmyBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建军事建筑
     * @param level 建筑等级
     * @return ArmyBuilding* 军事建筑指针
     */
    static ArmyBuilding* create(int level = 1);

    // 以下方法直接使用基类的配置驱动实现，无需重写：
    // - getMaxLevel()
    // - getUpgradeCost()
    // - getUpgradeTime()
    // - getMaxHitpoints()
    // - getBuildingDescription()
    // - getImageFile()

    // ==================== 训练系统 ====================

    /**
     * @brief 添加训练任务
     * @param unitType 要训练的兵种
     * @return bool 是否成功添加
     */
    bool addTrainingTask(UnitType unitType);

    /** @brief 取消当前训练任务 */
    void cancelCurrentTask();

    /** @brief 清空训练队列 */
    void clearTrainingQueue();

    /** @brief 获取训练队列长度 */
    int getQueueLength() const { return static_cast<int>(_trainingQueue.size()); }

    /**
     * @brief 获取当前训练进度
     * @return float 进度值 (0.0 ~ 1.0)
     */
    float getTrainingProgress() const;

    /**
     * @brief 设置训练完成回调
     * @param callback 回调函数
     */
    void setOnTrainingComplete(const std::function<void(BaseUnit*)>& callback) 
    { 
        _onTrainingComplete = callback; 
    }

    /**
     * @brief 每帧更新
     * @param dt 帧时间间隔
     */
    virtual void tick(float dt) override;

    /** @brief 获取当前可训练的士兵容量 */
    int getTrainingCapacity() const;

    /** @brief 获取训练速度加成 */
    float getTrainingSpeedBonus() const;

    // ==================== 静态方法：兵种数据 ====================

    /**
     * @brief 获取兵种训练时间
     * @param type 兵种类型
     * @return float 训练时间（秒）
     */
    static float getUnitBaseTrainingTime(UnitType type);

    /**
     * @brief 获取兵种训练费用
     * @param type 兵种类型
     * @return int 训练费用
     */
    static int getUnitTrainingCost(UnitType type);

    /**
     * @brief 获取兵种占用人口数
     * @param type 兵种类型
     * @return int 人口数
     */
    static int getUnitPopulation(UnitType type);

protected:
    /**
     * @brief 初始化兵营
     * @param level 初始等级
     * @return bool 是否成功
     */
    virtual bool init(int level) override;

    /** @brief 升级时调用 */
    virtual void onLevelUp() override;

    /**
     * @brief 通知军营显示训练好的小兵
     * @param type 兵种类型
     */
    void notifyArmyCampsToDisplayTroop(UnitType type);

private:
    std::queue<TrainingTask> _trainingQueue;             ///< 训练队列
    std::function<void(BaseUnit*)> _onTrainingComplete;  ///< 训练完成回调

    /** @brief 完成当前训练任务 */
    void completeCurrentTask();
};

#endif // ARMY_BUILDING_H_