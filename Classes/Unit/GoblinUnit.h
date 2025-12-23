/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GoblinUnit.h
 * File Function: 哥布林单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef GOBLIN_UNIT_H_
#define GOBLIN_UNIT_H_

#include "BaseUnit.h"

/**
 * @class GoblinUnit
 * @brief 哥布林单位类 - 快速单位，优先攻击资源建筑
 */
class GoblinUnit : public BaseUnit
{
public:
    static GoblinUnit* create(int level = 1);

    UnitType    getUnitType() const override { return UnitType::kGoblin; }
    std::string getDisplayName() const override { return "哥布林"; }

protected:
    bool init(int level) override;
    void loadAnimations() override;
};

#endif // GOBLIN_UNIT_H_