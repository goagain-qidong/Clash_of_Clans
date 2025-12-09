/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UpgradeTimerUI.cpp
 * File Function: 建筑升级倒计时UI实现
 * Author:        薛毓哲
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#include "UpgradeTimerUI.h"
#include "Buildings/BaseBuilding.h"
#include "Managers/UpgradeManager.h"

USING_NS_CC;

UpgradeTimerUI* UpgradeTimerUI::create(BaseBuilding* building)
{
    UpgradeTimerUI* ret = new (std::nothrow) UpgradeTimerUI();
    if (ret && ret->init(building))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool UpgradeTimerUI::init(BaseBuilding* building)
{
    if (!Node::init())
        return false;
    
    _building = building;
    
    // ?? 修复：正确的进度条尺寸和位置
    const float barWidth = 100.0f;
    const float barHeight = 10.0f;
    const float barY = 80.0f;  // 建筑上方
    
    // 创建进度条背景（深灰色边框）
    _progressBarBg = Sprite::create();
    _progressBarBg->setTextureRect(Rect(0, 0, barWidth, barHeight));
    _progressBarBg->setColor(Color3B(50, 50, 50));
    _progressBarBg->setAnchorPoint(Vec2(0.5f, 0.5f));  // ? 中心锚点
    _progressBarBg->setPosition(Vec2(0, barY));
    this->addChild(_progressBarBg, 1);
    
    // 创建进度条填充（绿色，从左到右填充）
    _progressBarFill = Sprite::create();
    _progressBarFill->setTextureRect(Rect(0, 0, barWidth, barHeight));
    _progressBarFill->setColor(Color3B(0, 255, 0));  // 绿色
    _progressBarFill->setAnchorPoint(Vec2(0, 0.5f));  // ? 左侧锚点，从左向右增长
    _progressBarFill->setPosition(Vec2(-barWidth / 2, 0));  // ? 修复：从背景左边缘开始（Y=0 因为背景锚点是中心）
    _progressBarBg->addChild(_progressBarFill, 1);
    
    // 创建时间文本
    _timeLabel = Label::createWithSystemFont("00:00", "Arial", 14);
    _timeLabel->setPosition(Vec2(0, barY + 20));  // 进度条上方
    _timeLabel->setTextColor(Color4B::WHITE);
    _timeLabel->enableOutline(Color4B::BLACK, 1);  // ? 添加黑色描边，提高可读性
    this->addChild(_timeLabel, 2);
    
    // 启用每帧更新
    scheduleUpdate();
    
    return true;
}

void UpgradeTimerUI::update(float dt)
{
    if (!_building)
        return;
    
    // 检查建筑是否还在升级中
    if (!_building->isUpgrading())
    {
        hide();
        return;
    }
    
    // 获取升级进度
    float progress = _building->getUpgradeProgress();
    float remainingTime = _building->getUpgradeRemainingTime();
    
    // 更新进度条
    _progressBarFill->setScaleX(progress);
    
    // 更新时间文本
    _timeLabel->setString(formatTime(remainingTime));
}

void UpgradeTimerUI::show()
{
    this->setVisible(true);
}

void UpgradeTimerUI::hide()
{
    this->setVisible(false);
}

std::string UpgradeTimerUI::formatTime(float seconds) const
{
    if (seconds <= 0)
        return "00:00";
    
    int totalSeconds = static_cast<int>(seconds);
    
    // 如果超过1天，显示天数
    if (totalSeconds >= 86400)
    {
        int days = totalSeconds / 86400;
        int hours = (totalSeconds % 86400) / 3600;
        return std::to_string(days) + "天 " + std::to_string(hours) + "时";
    }
    
    // 如果超过1小时，显示小时:分钟
    if (totalSeconds >= 3600)
    {
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        return std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes);
    }
    
    // 显示分钟:秒
    int minutes = totalSeconds / 60;
    int secs = totalSeconds % 60;
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, secs);
    return std::string(buffer);
}
