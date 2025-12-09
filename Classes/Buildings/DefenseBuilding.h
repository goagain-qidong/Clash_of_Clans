/****************************************************************
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DefenseBuilding.h
 * File Function: 防御建筑类（箭塔、加农炮等）
 * Author:        薛毓哲
 * Update Date:   2025/12/07
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseBuilding.h"
#include <vector>

// 前向声明：避免循环依赖
class Unit;

/**
 * @brief 防御建筑类型
 */
enum class DefenseType
{
    kCannon,      // 加农炮
    kArcherTower, // 箭塔
    kWizardTower, // 法师塔
    kAirDefense,  // 防空火炮
    kTesla        // 特斯拉电塔
};

/**
 * @class DefenseBuilding
 * @brief 防御建筑基类，具有自动攻击功能
 */
class DefenseBuilding : public BaseBuilding
{
public:
    static DefenseBuilding* create(DefenseType defenseType, int level = 1);
    static DefenseBuilding* create(DefenseType defenseType, int level, const std::string& imageFile);

    virtual bool init(DefenseType defenseType, int level);
    virtual bool init(DefenseType defenseType, int level, const std::string& imageFile);

    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kDefense; }
    virtual std::string  getDisplayName() const override;
    virtual int          getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kGold; }
    virtual float        getUpgradeTime() const override;
    virtual int          getMaxLevel() const override;
    virtual std::string  getBuildingDescription() const override;
    virtual bool         isDefenseBuilding() const override { return true; }

    // ==================== 防御功能 ====================

    /**
     * @brief 每帧更新（检测敌人、攻击）
     */
    virtual void tick(float dt) override;

    /**
     * @brief 检测范围内的敌人
     * @param units 场景中的所有单位
     */
    void detectEnemies(const std::vector<Unit*>& units);

    /**
     * @brief 攻击当前目标
     */
    virtual void attackTarget(Unit* target) override;

    /**
     * @brief 设置是否在战斗模式（只在战斗场景中激活）
     */
    void setBattleMode(bool enabled) { _battleModeEnabled = enabled; }

    /**
     * @brief 获取防御类型
     */
    DefenseType getDefenseType() const { return _defenseType; }

protected:
    virtual void        onLevelUp() override;
    virtual std::string getImageForLevel(int level) const override;

    /**
     * @brief 根据防御类型和等级初始化战斗属性
     */
    void initCombatStats();

    /**
     * @brief 发射投射物（炮弹、箭矢等）
     */
    void fireProjectile(Unit* target);

    /**
     * @brief 播放攻击动画
     */
    void playAttackAnimation();

private:
    DefenseType _defenseType = DefenseType::kCannon;  // 防御类型（给默认值）
    bool        _battleModeEnabled = false;           // 是否在战斗模式
    std::string _customImagePath;                     // 自定义图片路径
    std::string _customName;                          // 自定义名称
};

