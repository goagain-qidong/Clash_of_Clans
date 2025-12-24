/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingData.h
 * File Function: 建筑配置数据结构
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "BaseBuilding.h"
#include "cocos2d.h"

/**
 * @struct BuildingData
 * @brief 建筑配置数据
 */
struct BuildingData
{
    std::string name;        ///< 建筑名称
    std::string imageFile;   ///< 图片文件
    cocos2d::Size gridSize;  ///< 占用网格大小
    float scaleFactor;       ///< 缩放系数
    int cost;                ///< 建造费用
    int buildTime;           ///< 建造时间（秒）
    ResourceType costType;   ///< 消耗资源类型

    /** @brief 默认构造函数 */
    BuildingData()
        : name(""), imageFile(""), gridSize(cocos2d::Size::ZERO), scaleFactor(1.0f), cost(0), buildTime(0),
          costType(ResourceType::kGold)
    {}

    /**
     * @brief 带参数构造函数
     * @param n 建筑名称
     * @param img 图片文件
     * @param size 占用网格大小
     * @param scale 缩放系数
     * @param c 建造费用
     * @param time 建造时间
     * @param cType 消耗资源类型
     */
    BuildingData(const std::string& n, const std::string& img, const cocos2d::Size& size, float scale = 1.0f, int c = 0,
                 int time = 0, ResourceType cType = ResourceType::kGold)
        : name(n), imageFile(img), gridSize(size), scaleFactor(scale), cost(c), buildTime(time), costType(cType)
    {}
};