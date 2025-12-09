/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UpgradeTimerUI.h
 * File Function: 建筑升级倒计时UI（显示在建筑上方）
 * Author:        薛毓哲
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "cocos2d.h"

// 前向声明
class BaseBuilding;

/**
 * @class UpgradeTimerUI
 * @brief 升级倒计时UI组件（显示在建筑上方的进度条和剩余时间）
 */
class UpgradeTimerUI : public cocos2d::Node
{
public:
    static UpgradeTimerUI* create(BaseBuilding* building);
    
    virtual bool init(BaseBuilding* building);
    
    /**
     * @brief 每帧更新显示
     */
    virtual void update(float dt) override;
    
    /**
     * @brief 显示UI
     */
    void show();
    
    /**
     * @brief 隐藏UI
     */
    void hide();

private:
    BaseBuilding* _building = nullptr;           // 关联的建筑
    cocos2d::Sprite* _progressBarBg = nullptr;   // 进度条背景
    cocos2d::Sprite* _progressBarFill = nullptr; // 进度条填充
    cocos2d::Label* _timeLabel = nullptr;        // 时间文本
    
    /**
     * @brief 格式化剩余时间
     */
    std::string formatTime(float seconds) const;
};
