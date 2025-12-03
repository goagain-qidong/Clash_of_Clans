#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#ifndef __HERO_H__
#define __HERO_H__

#include "cocos2d.h"

class Hero : public cocos2d::Sprite
{
public:
    static Hero* create(const std::string& frameName);
    virtual bool init(const std::string& frameName);

    // 移动到指定位置（世界坐标）
    void moveTo(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode);

    // 通过路径移动（网格路径->世界坐标）
    void moveAlongPath(const std::vector<cocos2d::Vec2>& gridPath, cocos2d::Node* mapNode, class GridMap* grid);

    // 更新缩放（根据地图缩放调整）
    void updateScale(float mapScale);

    // 获取英雄名称
    std::string getHeroName() const { return _heroName; }

    // 设置/获取是否被选中
    void setSelected(bool selected);
    bool isSelected() const { return _isSelected; }

    // 播放行走动画
    void playWalkAnimation();
    void stopWalkAnimation();

    // 检查点击
    bool containsTouch(const cocos2d::Vec2& worldPosition, cocos2d::Node* mapNode);

private:
    std::string _heroName;
    bool _isSelected;
    float _baseScale; // 基础缩放比例
    bool _isMoving; // 是否正在移动

    // 创建行走动画帧序列
    void createWalkAnimationFrames();
};

#endif // __HERO_H__