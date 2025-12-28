/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GiantUnit.h
 * File Function: 巨人单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#ifndef GIANT_UNIT_H_
#define GIANT_UNIT_H_

#include "BaseUnit.h"

/**
 * @class GiantUnit
 * @brief 巨人单位类 - 高血量坦克，优先攻击防御建筑
 */
class GiantUnit : public BaseUnit
{
public:
    /**
     * @brief 创建巨人单位
     * @param level 单位等级
     * @return GiantUnit* 巨人指针
     */
    static GiantUnit* create(int level = 1);

    /** @brief 获取单位类型 */
    UnitType getUnitType() const override { return UnitType::kGiant; }

    /** @brief 获取显示名称 */
    std::string getDisplayName() const override { return "巨人"; }

protected:
    virtual bool init(int level) override;
    virtual void loadAnimations() override;

    // 音效回调
    void onAttackBefore() override;
    void onAttackAfter() override;
    void onDeathBefore() override;
};

#endif // GIANT_UNIT_H_