/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GiantUnit.h
 * File Function: 巨人单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/01/10
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
    static GiantUnit* create(int level = 1);

    UnitType    getUnitType() const override { return UnitType::kGiant; }
    std::string getDisplayName() const override { return "巨人"; }

protected:
    bool init(int level) override;
    void loadAnimations() override;
};

#endif // GIANT_UNIT_H_