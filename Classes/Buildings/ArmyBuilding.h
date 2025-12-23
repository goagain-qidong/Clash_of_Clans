/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyBuilding.h
 * File Function: 军事建筑类（兵营、训练营等）
 * Author:
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef ARMY_BUILDING_H_
#define ARMY_BUILDING_H_

#include "BaseBuilding.h"
#include "Unit/UnitTypes.h"

#include <functional>
#include <queue>

// Forward declaration
class BaseUnit;

/**
 * @struct TrainingTask
 * @brief 训练任务结构体
 */
struct TrainingTask
{
    UnitType unitType;     // 兵种类型
    float    trainingTime; // 训练时间（秒）
    float    elapsedTime;  // 已经过的时间
    int      cost;         // 训练费用

    TrainingTask(UnitType type, float time, int costValue)
        : unitType(type), trainingTime(time), elapsedTime(0.0f), cost(costValue)
    {}
};

/**
 * @class ArmyBuilding
 * @brief 军事建筑类，用于训练士兵
 */
class ArmyBuilding : public BaseBuilding
{
public:
    static ArmyBuilding* create(int level = 1);
    static ArmyBuilding* create(int level, const std::string& imageFile);
    virtual bool         init(int level);
    virtual bool         init(int level, const std::string& imageFile);

    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kArmy; }
    virtual std::string  getDisplayName() const override;
    virtual int          getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kElixir; }
    virtual float        getUpgradeTime() const override;
    virtual int          getMaxLevel() const override { return 14; }
    virtual std::string  getBuildingDescription() const override;

    // ==================== 训练系统 ====================

    /**
     * @brief 添加训练任务
     * @param unitType 要训练的兵种
     * @return 是否成功添加
     */
    bool addTrainingTask(UnitType unitType);

    /**
     * @brief 取消当前训练任务
     */
    void cancelCurrentTask();

    /**
     * @brief 清空训练队列
     */
    void clearTrainingQueue();

    /**
     * @brief 获取训练队列长度
     */
    int getQueueLength() const { return static_cast<int>(_trainingQueue.size()); }

    /**
     * @brief 获取当前训练进度（0.0 ~ 1.0）
     */
    float getTrainingProgress() const;

    /**
     * @brief 设置训练完成回调
     */
    void setOnTrainingComplete(const std::function<void(BaseUnit*)>& callback) { _onTrainingComplete = callback; }

    /**
     * @brief 每帧更新（处理训练逻辑）
     */
    virtual void tick(float dt) override;

    // ==================== 兵营特有功能 ====================
    /** @brief 获取当前可训练的士兵容量 */
    int getTrainingCapacity() const;
    /** @brief 获取训练速度加成 (百分比) */
    float getTrainingSpeedBonus() const;

    /**
     * @brief 获取兵种训练时间（基础时间）
     */
    static float getUnitBaseTrainingTime(UnitType type);

    /**
     * @brief 获取兵种训练费用
     */
    static int getUnitTrainingCost(UnitType type);

    /**
     * @brief 获取兵种占用人口数
     */
    static int getUnitPopulation(UnitType type);

protected:
    virtual void        onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

    // 🆕 通知军营显示训练好的小兵
    void notifyArmyCampsToDisplayTroop(UnitType type);

private:
    std::string _customImagePath; // 自定义图片路径（用于箭塔、炮塔等）
    std::string _customName;      // 自定义建筑名称

    // ==================== 训练系统私有成员 ====================
    std::queue<TrainingTask>       _trainingQueue;      // 训练队列
    std::function<void(BaseUnit*)> _onTrainingComplete; // 训练完成回调

    /**
     * @brief 完成当前训练任务
     */
    void completeCurrentTask();
};

#endif // ARMY_BUILDING_H_