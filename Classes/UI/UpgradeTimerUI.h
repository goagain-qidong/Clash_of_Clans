/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UpgradeTimerUI.h
 * File Function: 建筑升级倒计时UI
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"

class BaseBuilding;

/**
 * @class UpgradeTimerUI
 * @brief 建筑升级倒计时UI
 *
 * 显示：倒计时文本 + 绿色进度条
 */
class UpgradeTimerUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建倒计时UI
     * @param building 关联的建筑
     * @return UpgradeTimerUI* UI指针
     */
    static UpgradeTimerUI* create(BaseBuilding* building);

    /**
     * @brief 初始化
     * @param building 关联的建筑
     * @return bool 是否成功
     */
    virtual bool init(BaseBuilding* building);

    virtual void update(float dt) override;

private:
    BaseBuilding* _building = nullptr;    ///< 关联的建筑
    cocos2d::Sprite* _barBg = nullptr;    ///< 灰色背景
    cocos2d::Sprite* _barFill = nullptr;  ///< 绿色填充
    cocos2d::Label* _timeLabel = nullptr; ///< 剩余时间标签

    /**
     * @brief 创建纯色纹理精灵
     * @param size 尺寸
     * @param color 颜色
     * @return cocos2d::Sprite* 精灵指针
     */
    cocos2d::Sprite* createColorSprite(const cocos2d::Size& size, const cocos2d::Color3B& color);

    /**
     * @brief 格式化时间
     * @param seconds 秒数
     * @return std::string 格式化字符串
     */
    std::string formatTime(float seconds);
};