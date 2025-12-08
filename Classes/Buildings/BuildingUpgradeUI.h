/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeUI.h
 * File Function: 通用建筑升级界面
 * Author:        赵崇沛
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/

#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "BaseBuilding.h"

// 前向声明
class ArmyBuilding;

/**
 * @class BuildingUpgradeUI
 * @brief 通用建筑升级界面，适用于所有建筑类型
 */
class BuildingUpgradeUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建升级界面
     * @param building 目标建筑
     */
    static BuildingUpgradeUI* create(BaseBuilding* building);

    virtual bool init(BaseBuilding* building);

    /** @brief 设置位置到建筑附近 */
    void setPositionNearBuilding(BaseBuilding* building);

    /** @brief 显示界面 */
    void show();

    /** @brief 隐藏界面 */
    void hide();

    using UpgradeResultCallback = std::function<void(bool success, int newLevel)>;
    void setUpgradeCallback(const UpgradeResultCallback& callback) { _resultCallback = callback; }

    using CloseCallback = std::function<void()>;
    void setCloseCallback(const CloseCallback& cb) { _closeCallback = cb; }

private:
    void setupUI();
    void updateUI();
    void onUpgradeClicked();
    void onCloseClicked();
    void onTrainClicked();  // 新增：点击训练按钮

    std::string getResourceTypeName(ResourceType type) const;
    std::string formatTime(int seconds) const;

private:
    BaseBuilding* _building = nullptr;
    cocos2d::ui::Layout* _panel = nullptr;
    cocos2d::Label* _titleLabel = nullptr;
    cocos2d::Label* _levelLabel = nullptr;
    cocos2d::Label* _descLabel = nullptr;
    cocos2d::Label* _costLabel = nullptr;
    cocos2d::Label* _timeLabel = nullptr;
    cocos2d::ui::Button* _upgradeButton = nullptr;
    cocos2d::ui::Button* _closeButton = nullptr;
    cocos2d::ui::Button* _trainButton = nullptr;  // 新增：训练按钮（仅兵营显示）

    UpgradeResultCallback _resultCallback = nullptr;
    CloseCallback _closeCallback = nullptr;
};