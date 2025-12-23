/****************************************************************
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     CombatStats.h
 * File Function: 战斗属性系统 - 定义所有战斗实体的基础属性
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef COMBAT_STATS_H_
#define COMBAT_STATS_H_

/**
 * @brief 战斗属性结构体
 * 适用于建筑和单位
 */
struct CombatStats
{
    // ==================== 生命值 ====================
    int maxHitpoints     = 100; // 最大生命值
    int currentHitpoints = 100; // 当前生命值

    // ==================== 攻击属性 ====================
    float damage      = 0.0f; // 每次攻击伤害（支持小数）
    float attackSpeed = 1.0f; // 攻击速度（秒/次）
    float attackRange = 0.0f; // 攻击范围（像素）

    // ==================== 防御属性 ====================
    int armor = 0; // 护甲（减少伤害）

    // ==================== 目标类型 ====================
    enum class TargetType
    {
        kAny,      // 任何目标
        kGround,   // 仅地面目标
        kAir,      // 仅空中目标
        kDefense,  // 优先防御建筑
        kResource, // 优先资源建筑
        kWalls     // 仅城墙
    };

    TargetType preferredTarget = TargetType::kAny; // 优先目标类型

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
        if (maxHitpoints <= 0)
            return 0.0f;
        return static_cast<float>(currentHitpoints) / maxHitpoints;
    }

    /**
     * @brief 受到伤害
     * @param dmg 伤害值
     * @return 实际受到的伤害（考虑护甲）
     */
    float takeDamage(float dmg)
    {
        float actualDamage = dmg - armor;
        if (actualDamage < 1.0f)
            actualDamage = 1.0f; // 至少造成1点伤害

        currentHitpoints -= static_cast<int>(actualDamage + 0.5f); // 四舍五入
        if (currentHitpoints < 0)
            currentHitpoints = 0;

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
    void fullHeal() { currentHitpoints = maxHitpoints; }
};

/**
 * @brief 单位配置表
 */
namespace UnitConfig
{
// 野蛮人配置（根据官方数据调整）
inline CombatStats getBarbarian(int level)
{
    CombatStats stats;
    // 等级1数据：DPS 9, 每次伤害 9, HP 45
    stats.maxHitpoints     = 45;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 9;
    stats.attackSpeed      = 1.0f;  // 1秒攻击一次
    stats.attackRange      = 50.0f; // 近战范围
    stats.preferredTarget  = CombatStats::TargetType::kAny;
    return stats;
}

// 弓箭手配置（根据官方数据）
inline CombatStats getArcher(int level)
{
    CombatStats stats;
    // 等级1数据：DPS 8, 每次伤害 8, HP 22
    stats.maxHitpoints     = 22;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 8;
    stats.attackSpeed      = 1.0f;   // 1秒攻击一次
    stats.attackRange      = 200.0f; // 远程攻击
    stats.preferredTarget  = CombatStats::TargetType::kAny;
    return stats;
}

// 巨人配置（根据官方数据）
inline CombatStats getGiant(int level)
{
    CombatStats stats;
    // 等级1数据：DPS 12, 每次伤害 24, HP 400, 攻速 2秒/次
    // 特点：血厚、攻击力高、攻速慢、优先攻击防御建筑
    stats.maxHitpoints     = 400;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 24;
    stats.attackSpeed      = 2.0f; // 2秒攻击一次
    stats.attackRange      = 50.0f;
    stats.preferredTarget  = CombatStats::TargetType::kDefense; // 优先打防御
    return stats;
}

// 哥布林配置（根据官方数据）
inline CombatStats getGoblin(int level)
{
    CombatStats stats;
    // 等级1数据：DPS 11, 每次伤害 11, HP 25
    // 特点：优先攻击资源建筑，对资源建筑造成2倍伤害（22）
    stats.maxHitpoints     = 25;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 11;
    stats.attackSpeed      = 1.0f; // 1秒攻击一次
    stats.attackRange      = 50.0f;
    stats.preferredTarget  = CombatStats::TargetType::kResource; // 优先打资源
    return stats;
}

// 炸弹人配置（调整为合理数值）
inline CombatStats getWallBreaker(int level)
{
    CombatStats stats;
    // 炸弹人特点：血少、自爆伤害极高、专门炸城墙
    stats.maxHitpoints     = 20;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 150;  // 自爆伤害高
    stats.attackSpeed      = 0.0f; // 自爆型
    stats.attackRange      = 30.0f;
    stats.preferredTarget  = CombatStats::TargetType::kWalls;
    return stats;
}
} // namespace UnitConfig

/**
 * @brief 防御建筑配置表
 */
namespace DefenseConfig
{
// 加农炮配置（根据官方数据调整，最高14级）
inline CombatStats getCannon(int level)
{
    CombatStats stats;

    // 生命值配置（等级1-14）
    const int hitpointsTable[] = {300, 360, 420, 500, 600, 660, 730, 800, 880, 960, 1060, 1160, 1260, 1380};

    // 每次伤害配置（等级1-14）
    const float damageTable[] = {5.6f,  8.0f,  10.4f, 13.6f, 18.4f, 24.0f, 32.0f,
                                 38.4f, 44.8f, 51.2f, 59.2f, 68.0f, 76.0f, 80.0f};

    // 限制等级范围 1-14
    int actualLevel = level;
    if (actualLevel < 1)
        actualLevel = 1;
    if (actualLevel > 14)
        actualLevel = 14;

    stats.maxHitpoints     = hitpointsTable[actualLevel - 1];
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = damageTable[actualLevel - 1];     // 直接使用float值
    stats.attackSpeed      = 0.8f;                             // 固定0.8秒攻击一次
    stats.attackRange      = 250.0f;                           // 攻击范围250像素
    stats.preferredTarget  = CombatStats::TargetType::kGround; // 只攻击地面目标
    return stats;
}

// 箭塔配置（根据官方数据调整）
inline CombatStats getArcherTower(int level)
{
    CombatStats stats;

    // 生命值配置（等级1-14）
    const int hitpointsTable[] = {380, 420, 460, 500, 540, 580, 630, 690, 750, 810, 890, 970, 1050, 1130};

    // 每次伤害配置（等级1-14）
    const float damageTable[] = {5.5f,  7.5f,  9.5f,  12.5f, 15.0f, 17.5f, 21.0f,
                                 24.0f, 28.0f, 31.5f, 35.0f, 37.0f, 39.0f, 41.0f};

    // 限制等级范围 1-14
    int actualLevel = level;
    if (actualLevel < 1)
        actualLevel = 1;
    if (actualLevel > 14)
        actualLevel = 14;

    stats.maxHitpoints     = hitpointsTable[actualLevel - 1];
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = damageTable[actualLevel - 1];  // 直接使用float值，保留小数
    stats.attackSpeed      = 0.5f;                          // 固定0.5秒攻击一次
    stats.attackRange      = 300.0f;                        // 攻击范围300像素（比加农炮远）
    stats.preferredTarget  = CombatStats::TargetType::kAny; // 可攻击空中和地面目标
    return stats;
}

// 法师塔配置
inline CombatStats getWizardTower(int level)
{
    CombatStats stats;
    stats.maxHitpoints     = 620 + (level - 1) * 90;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 16 + (level - 1) * 4; // 范围伤害
    stats.attackSpeed      = 1.5f;
    stats.attackRange      = 250.0f;
    stats.preferredTarget  = CombatStats::TargetType::kAny;
    return stats;
}

// 城墙配置
inline CombatStats getWall(int level)
{
    CombatStats stats;
    stats.maxHitpoints     = 300 + (level - 1) * 100; // 纯防御
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 0; // 不攻击
    stats.attackSpeed      = 0.0f;
    stats.attackRange      = 0.0f;
    return stats;
}
} // namespace DefenseConfig

/**
 * @brief 其他建筑配置
 */
namespace BuildingConfig
{
// 大本营配置
inline CombatStats getTownHall(int level)
{
    CombatStats stats;
    stats.maxHitpoints     = 1500 + (level - 1) * 500; // 血量最高
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 0;
    stats.attackSpeed      = 0.0f;
    stats.attackRange      = 0.0f;
    return stats;
}

// 资源建筑（金矿、圣水收集器等）
inline CombatStats getResourceBuilding(int level)
{
    CombatStats stats;
    stats.maxHitpoints     = 400 + (level - 1) * 50;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 0;
    stats.attackSpeed      = 0.0f;
    stats.attackRange      = 0.0f;
    return stats;
}

// 仓库（金库、圣水仓库）
inline CombatStats getStorageBuilding(int level)
{
    CombatStats stats;
    stats.maxHitpoints     = 600 + (level - 1) * 100;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 0;
    stats.attackSpeed      = 0.0f;
    stats.attackRange      = 0.0f;
    return stats;
}

// 兵营
inline CombatStats getBarracks(int level)
{
    CombatStats stats;
    stats.maxHitpoints     = 500 + (level - 1) * 80;
    stats.currentHitpoints = stats.maxHitpoints;
    stats.damage           = 0;
    stats.attackSpeed      = 0.0f;
    stats.attackRange      = 0.0f;
    return stats;
}
} // namespace BuildingConfig

#endif // COMBAT_STATS_H_
