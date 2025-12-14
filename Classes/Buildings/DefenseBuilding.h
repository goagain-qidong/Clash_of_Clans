/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DefenseBuilding.h
 * File Function: 防御建筑类
 * Author:        薛毓哲
 * Update Date:   2025/12/07
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "BaseBuilding.h"
#include "cocos2d.h"
#include <string>
#include <vector>

// Forward declaration
class Unit;
class BuildingHealthBarUI;

enum class DefenseType
{
    kCannon,
    kArcherTower,
    kWizardTower
};

/**
 * @class DefenseBuilding
 * @brief 防御建筑基类
 */
class DefenseBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建防御建筑
     */
    static DefenseBuilding* create(DefenseType defenseType, int level);

    /**
     * @brief 创建防御建筑（自定义图片）
     */
    static DefenseBuilding* create(DefenseType defenseType, int level, const std::string& imageFile);

    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kDefense; }
    virtual std::string  getDisplayName() const override;
    virtual int          getMaxLevel() const override;
    virtual int          getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kGold; }
    virtual float        getUpgradeTime() const override;
    virtual std::string  getBuildingDescription() const override;
    virtual std::string  getImageForLevel(int level) const override;

    // ==================== 战斗逻辑 ====================
    virtual void tick(float dt) override;

    /**
     * @brief 检测敌方士兵并自动选择目标
     */
    void detectEnemies(const std::vector<Unit*>& units);

    /**
     * @brief 攻击目标
     */
    virtual void attackTarget(Unit* target) override;

    /**
     * @brief 发射炮弹并造成伤害
     */
    void fireProjectile(Unit* target);

    /**
     * @brief 播放攻击动画
     */
    void playAttackAnimation();

    

    /**
     * @brief 检查是否在战斗模式中
     */
    bool isBattleModeEnabled() const { return _battleModeEnabled; }
    
    /**
     * @brief 显示攻击范围（半透明圆圈）
     */
    void showAttackRange();
    
    /**
     * @brief 隐藏攻击范围
     */
    void hideAttackRange();
    
    /**
     * @brief 旋转建筑朝向目标
     */
    void rotateToTarget(const cocos2d::Vec2& targetPos);
    
    /**
     * @brief 获取防御建筑类型
     */
    DefenseType getDefenseType() const { return _defenseType; }

protected:
    virtual bool init(DefenseType defenseType, int level);
    virtual bool init(DefenseType defenseType, int level, const std::string& imageFile);
    virtual void onLevelUp() override;

private:
    // ==================== 初始化方法 ====================
    void initCombatStats();
    
    // ==================== 视觉效果 ====================
    /**
     * @brief 创建炮弹精灵（加农炮）
     */
    cocos2d::Sprite* createCannonballSprite();
    
    /**
     * @brief 创建箭矢精灵（箭塔）
     */
    cocos2d::Sprite* createArrowSprite();

    // ==================== 防御建筑属性 ====================
    DefenseType _defenseType;
    std::string _customImagePath;
    std::string _customName;
    
    // ==================== 攻击范围显示 ====================
    cocos2d::DrawNode* _rangeCircle = nullptr;  // 攻击范围圆圈
    
};