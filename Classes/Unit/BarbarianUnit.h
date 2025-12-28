/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BarbarianUnit.h
 * File Function: 野蛮人单位类
 * Author:        薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#ifndef BARBARIAN_UNIT_H_
#define BARBARIAN_UNIT_H_

#include "BaseUnit.h"

/**
 * @class BarbarianUnit
 * @brief 野蛮人单位类 - 近战单位
 */
class BarbarianUnit : public BaseUnit
{
public:
    /**
     * @brief 创建野蛮人单位
     * @param level 单位等级
     * @return BarbarianUnit* 野蛮人指针
     */
    static BarbarianUnit* create(int level = 1);

    /** @brief 获取单位类型 */
    UnitType getUnitType() const override { return UnitType::kBarbarian; }

    /** @brief 获取显示名称 */
    std::string getDisplayName() const override { return "野蛮人"; }

protected:
    virtual bool init(int level) override;
    virtual void loadAnimations() override;

    // 音效回调
    virtual void onAttackBefore() override;
    virtual void onDeathBefore() override;
};

#endif // BARBARIAN_UNIT_H_