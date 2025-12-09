/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     CombatStats.h
 * File Function: 战斗属性系统 - 定义所有战斗实体的基础属性
 * Author:        薛毓哲
 * Update Date:   2025/12/07
 * License:       MIT License
 ****************************************************************/
#pragma once

/**
 * @brief 战斗属性结构体
 * 适用于建筑和单位
 */
struct CombatStats
{
    // ==================== 生命值 ====================
    int maxHitpoints = 100;        // 最大生命值
    int currentHitpoints = 100;    // 当前生命值
    
    // ==================== 攻击属性 ====================
    int damage = 0;                // 每次攻击伤害
    float attackSpeed = 1.0f;      // 攻击速度（秒/次）
    float attackRange = 0.0f;      // 攻击范围（像素）
    
    // ==================== 防御属性 ====================
    int armor = 0;                 // 护甲（减少伤害）
    
    // ==================== 目标类型 ====================
    enum class TargetType
    {
        kAny,           // 任何目标
        kGround,        // 仅地面目标
        kAir,           // 仅空中目标
        kDefense,       // 优先防御建筑
        kResource,      // 优先资源建筑
        kWalls          // 仅城墙
    };
    
    TargetType preferredTarget = TargetType::kAny;  // 优先目标类型
    
    // ==================== 辅助方法 ====================
    
    /**
     * @brief 是否还活着
     */
    bool isAlive() const { return currentHitpoints > 0; }
    
    /**
     * @brief 生命值百分比
     */
    float getHealthPercent() const 
    { 
        if (maxHitpoints <= 0) return 0.0f;
        return static_cast<float>(currentHitpoints) / maxHitpoints; 
    }
    
    /**
     * @brief 受到伤害
     * @param dmg 伤害值
     * @return 实际受到的伤害（考虑护甲）
     */
    int takeDamage(int dmg)
    {
        int actualDamage = dmg - armor;
        if (actualDamage < 1) actualDamage = 1; // 至少造成1点伤害
        
        currentHitpoints -= actualDamage;
        if (currentHitpoints < 0) currentHitpoints = 0;
        
        return actualDamage;
    }
    
    /**
     * @brief 治疗
     */
    void heal(int amount)
    {
        currentHitpoints += amount;
        if (currentHitpoints > maxHitpoints) 
            currentHitpoints = maxHitpoints;
    }
    
    /**
     * @brief 完全恢复
     */
    void fullHeal()
    {
        currentHitpoints = maxHitpoints;
    }
};

/**
 * @brief 单位配置表
 */
namespace UnitConfig
{
    // 野蛮人配置
    inline CombatStats getBarbarian(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 45 + (level - 1) * 5;      // 等级1: 45HP
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 8 + (level - 1) * 2;              // 等级1: 8 DPS
        stats.attackSpeed = 1.0f;                        // 1秒攻击一次
        stats.attackRange = 50.0f;                       // 近战范围
        stats.preferredTarget = CombatStats::TargetType::kAny;
        return stats;
    }
    
    // 弓箭手配置
    inline CombatStats getArcher(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 20 + (level - 1) * 3;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 7 + (level - 1) * 2;
        stats.attackSpeed = 1.0f;
        stats.attackRange = 200.0f;                      // 远程攻击
        stats.preferredTarget = CombatStats::TargetType::kAny;
        return stats;
    }
    
    // 巨人配置
    inline CombatStats getGiant(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 300 + (level - 1) * 40;     // 血厚
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 11 + (level - 1) * 3;
        stats.attackSpeed = 2.0f;                        // 攻击慢
        stats.attackRange = 50.0f;
        stats.preferredTarget = CombatStats::TargetType::kDefense; // 优先打防御
        return stats;
    }
    
    // 哥布林配置
    inline CombatStats getGoblin(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 25 + (level - 1) * 3;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 11 + (level - 1) * 2;
        stats.attackSpeed = 1.0f;
        stats.attackRange = 50.0f;
        stats.preferredTarget = CombatStats::TargetType::kResource; // 优先打资源
        return stats;
    }
    
    // 炸弹人配置
    inline CombatStats getWallBreaker(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 20 + (level - 1) * 4;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 150 + (level - 1) * 50;           // 自爆伤害高
        stats.attackSpeed = 0.0f;                        // 自爆型
        stats.attackRange = 30.0f;
        stats.preferredTarget = CombatStats::TargetType::kWalls;
        return stats;
    }
}

/**
 * @brief 防御建筑配置表
 */
namespace DefenseConfig
{
    // 加农炮配置
    inline CombatStats getCannon(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 380 + (level - 1) * 60;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 9 + (level - 1) * 2;
        stats.attackSpeed = 0.8f;
        stats.attackRange = 250.0f;
        stats.preferredTarget = CombatStats::TargetType::kGround;
        return stats;
    }
    
    // 箭塔配置
    inline CombatStats getArcherTower(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 380 + (level - 1) * 60;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 11 + (level - 1) * 3;
        stats.attackSpeed = 1.0f;
        stats.attackRange = 300.0f;
        stats.preferredTarget = CombatStats::TargetType::kAny; // 可打空中和地面
        return stats;
    }
    
    // 法师塔配置
    inline CombatStats getWizardTower(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 620 + (level - 1) * 90;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 16 + (level - 1) * 4;             // 范围伤害
        stats.attackSpeed = 1.5f;
        stats.attackRange = 250.0f;
        stats.preferredTarget = CombatStats::TargetType::kAny;
        return stats;
    }
    
    // 城墙配置
    inline CombatStats getWall(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 300 + (level - 1) * 100;    // 纯防御
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 0;                                 // 不攻击
        stats.attackSpeed = 0.0f;
        stats.attackRange = 0.0f;
        return stats;
    }
}

/**
 * @brief 其他建筑配置
 */
namespace BuildingConfig
{
    // 大本营配置
    inline CombatStats getTownHall(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 1500 + (level - 1) * 500;   // 血量最高
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 0;
        stats.attackSpeed = 0.0f;
        stats.attackRange = 0.0f;
        return stats;
    }
    
    // 资源建筑（金矿、圣水收集器等）
    inline CombatStats getResourceBuilding(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 400 + (level - 1) * 50;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 0;
        stats.attackSpeed = 0.0f;
        stats.attackRange = 0.0f;
        return stats;
    }
    
    // 仓库（金库、圣水仓库）
    inline CombatStats getStorageBuilding(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 600 + (level - 1) * 100;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 0;
        stats.attackSpeed = 0.0f;
        stats.attackRange = 0.0f;
        return stats;
    }
    
    // 兵营
    inline CombatStats getBarracks(int level)
    {
        CombatStats stats;
        stats.maxHitpoints = 500 + (level - 1) * 80;
        stats.currentHitpoints = stats.maxHitpoints;
        stats.damage = 0;
        stats.attackSpeed = 0.0f;
        stats.attackRange = 0.0f;
        return stats;
    }
}
