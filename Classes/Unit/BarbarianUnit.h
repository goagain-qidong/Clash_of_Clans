/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BarbarianUnit.h
 * File Function: 野蛮人单位类
 * Author:        薛毓哲
 * Update Date:   2025/01/10
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
    static BarbarianUnit* create(int level = 1);

    UnitType    getUnitType() const override { return UnitType::kBarbarian; }
    std::string getDisplayName() const override { return "野蛮人"; }

protected:
    bool init(int level) override;
    void loadAnimations() override;
};

#endif // BARBARIAN_UNIT_H_