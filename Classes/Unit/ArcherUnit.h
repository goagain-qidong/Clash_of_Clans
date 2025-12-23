/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArcherUnit.h
 * File Function: 弓箭手单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef ARCHER_UNIT_H_
#define ARCHER_UNIT_H_

#include "BaseUnit.h"

/**
 * @class ArcherUnit
 * @brief 弓箭手单位类 - 远程单位
 */
class ArcherUnit : public BaseUnit
{
public:
    static ArcherUnit* create(int level = 1);

    UnitType    getUnitType() const override { return UnitType::kArcher; }
    std::string getDisplayName() const override { return "弓箭手"; }

protected:
    bool init(int level) override;
    void loadAnimations() override;
};

#endif // ARCHER_UNIT_H_