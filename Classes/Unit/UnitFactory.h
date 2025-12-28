/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UnitFactory.h
 * File Function: 单位工厂 - 统一创建单位
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#ifndef UNIT_FACTORY_H_
#define UNIT_FACTORY_H_

#include "BaseUnit.h"
#include "UnitTypes.h"

#include <string>

/**
 * @class UnitFactory
 * @brief 单位工厂类 - 提供统一的单位创建接口
 */
class UnitFactory
{
public:
    /**
     * @brief 创建单位
     * @param type 单位类型
     * @param level 单位等级
     * @return BaseUnit* 单位指针
     */
    static BaseUnit* createUnit(UnitType type, int level = 1);

    /**
     * @brief 获取兵种人口数
     * @param type 单位类型
     * @return int 人口数
     */
    static int getUnitPopulation(UnitType type);

    /**
     * @brief 获取兵种名称
     * @param type 单位类型
     * @return std::string 兵种名称
     */
    static std::string getUnitName(UnitType type);
};

#endif // UNIT_FACTORY_H_