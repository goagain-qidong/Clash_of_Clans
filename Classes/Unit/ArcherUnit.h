/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArcherUnit.h
 * File Function: 弓箭手单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#ifndef ARCHER_UNIT_H_
#define ARCHER_UNIT_H_

#include "BaseUnit.h"

/**
 * @class ArcherUnit
 * @brief 弓箭手单位类 - 远程单位
 */
class ArcherUnit : public BaseUnit {
 public:
  /**
   * @brief 创建弓箭手单位
   * @param level 单位等级
   * @return ArcherUnit* 弓箭手指针
   */
  static ArcherUnit* create(int level = 1);

  /** @brief 获取单位类型 */
  UnitType getUnitType() const override { return UnitType::kArcher; }

  /** @brief 获取显示名称 */
  std::string getDisplayName() const override { return "弓箭手"; }

 protected:
  bool init(int level) override;
  void loadAnimations() override;

  // 音效回调
  void onAttackBefore() override;
  void onDeathBefore() override;
};

#endif  // ARCHER_UNIT_H_