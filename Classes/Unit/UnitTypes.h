/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UnitTypes.h
 * File Function: 单位类型定义 - 所有单位相关的枚举和常量
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef UNIT_TYPES_H_
#define UNIT_TYPES_H_

/**
 * @enum UnitType
 * @brief 单位类型枚举
 */
enum class UnitType {
  kBarbarian,    // 野蛮人
  kArcher,       // 弓箭手
  kGiant,        // 巨人
  kGoblin,       // 哥布林
  kWallBreaker   // 炸弹人
};

/**
 * @enum UnitAction
 * @brief 单位动作枚举
 */
enum class UnitAction {
  kRun,      // 跑步
  kIdle,     // 待机
  kAttack,   // 攻击1
  kAttack2,  // 攻击2
  kDeath     // 死亡
};

/**
 * @enum UnitDirection
 * @brief 单位朝向枚举（支持8方向）
 */
enum class UnitDirection {
  kUp,
  kUpRight,
  kRight,
  kDownRight,
  kDown,
  kDownLeft,
  kLeft,
  kUpLeft
};

#endif  // UNIT_TYPES_H_