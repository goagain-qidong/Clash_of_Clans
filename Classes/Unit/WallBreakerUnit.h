/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBreakerUnit.h
 * File Function: 炸弹人单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef WALL_BREAKER_UNIT_H_
#define WALL_BREAKER_UNIT_H_

#include "BaseUnit.h"

/**
 * @class WallBreakerUnit
 * @brief 炸弹人单位类
 *
 * 特点：
 * - 优先攻击城墙
 * - 自杀式攻击：接触目标时爆炸
 * - 对城墙造成40倍伤害
 * - 死亡时不留墓碑
 */
class WallBreakerUnit : public BaseUnit
{
public:
    static WallBreakerUnit* create(int level = 1);

    UnitType    getUnitType() const override { return UnitType::kWallBreaker; }
    std::string getDisplayName() const override { return "炸弹人"; }

protected:
    bool init(int level) override;
    void loadAnimations() override;

    // 炸弹人特殊行为：死亡时爆炸
    void onDeathBefore() override;

private:
    // 创建爆炸视觉效果
    void createExplosionEffect();
};

#endif // WALL_BREAKER_UNIT_H_