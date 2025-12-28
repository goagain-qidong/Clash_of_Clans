/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBreakerUnit.h
 * File Function: 炸弹人单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#ifndef WALL_BREAKER_UNIT_H_
#define WALL_BREAKER_UNIT_H_

#include "BaseUnit.h"

/**
 * @class WallBreakerUnit
 * @brief 炸弹人单位类 - 自杀式攻击，优先攻击城墙
 *
 * 特点：
 * - 优先攻击城墙
 * - 自杀式攻击：接触目标时爆炸
 * - 对城墙造成40倍伤害
 * - 死亡时不留墓碑
 */
class WallBreakerUnit : public BaseUnit {
 public:
  /**
   * @brief 创建炸弹人单位
   * @param level 单位等级
   * @return WallBreakerUnit* 炸弹人指针
   */
  static WallBreakerUnit* create(int level = 1);

  /** @brief 获取单位类型 */
  UnitType getUnitType() const override { return UnitType::kWallBreaker; }

  /** @brief 获取显示名称 */
  std::string getDisplayName() const override { return "炸弹人"; }

 protected:
  bool init(int level) override;
  void loadAnimations() override;

  // 音效回调
  void onAttackBefore() override;
  void onDeathBefore() override;  ///< 死亡时爆炸

 private:
  void createExplosionEffect();  ///< 创建爆炸视觉效果
};

#endif  // WALL_BREAKER_UNIT_H_