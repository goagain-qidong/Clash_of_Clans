/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GoblinUnit.h
 * File Function: 哥布林单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#ifndef GOBLIN_UNIT_H_
#define GOBLIN_UNIT_H_

#include "BaseUnit.h"

/**
 * @class GoblinUnit
 * @brief 哥布林单位类 - 快速单位，优先攻击资源建筑
 */
class GoblinUnit : public BaseUnit {
 public:
  /**
   * @brief 创建哥布林单位
   * @param level 单位等级
   * @return GoblinUnit* 哥布林指针
   */
  static GoblinUnit* create(int level = 1);

  /** @brief 获取单位类型 */
  UnitType getUnitType() const override { return UnitType::kGoblin; }

  /** @brief 获取显示名称 */
  std::string getDisplayName() const override { return "哥布林"; }

 protected:
  bool init(int level) override;
  void loadAnimations() override;

  // 音效回调
  void onAttackBefore() override;
  void onDeathBefore() override;
};

#endif  // GOBLIN_UNIT_H_