/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeUI.h
 * File Function: 通用建筑升级界面
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "BaseBuilding.h"

class ArmyBuilding;

/**
 * @class BuildingUpgradeUI
 * @brief 通用建筑升级界面 - 适用于所有建筑类型
 */
class BuildingUpgradeUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建升级界面
     * @param building 目标建筑
     * @return BuildingUpgradeUI* 升级界面指针
     */
    static BuildingUpgradeUI* create(BaseBuilding* building);

    /**
     * @brief 初始化
     * @param building 目标建筑
     * @return bool 初始化是否成功
     */
    virtual bool init(BaseBuilding* building);

    /**
     * @brief 设置位置到建筑附近
     * @param building 目标建筑
     */
    void setPositionNearBuilding(BaseBuilding* building);

    /** @brief 显示界面 */
    void show();

    /** @brief 隐藏界面 */
    void hide();

    using UpgradeResultCallback = std::function<void(bool success, int newLevel)>;

    /**
     * @brief 设置升级回调
     * @param callback 回调函数
     */
    void setUpgradeCallback(const UpgradeResultCallback& callback) { _resultCallback = callback; }

    using CloseCallback = std::function<void()>;

    /**
     * @brief 设置关闭回调
     * @param cb 回调函数
     */
    void setCloseCallback(const CloseCallback& cb) { _closeCallback = cb; }

private:
    void setupUI();       ///< 设置UI
    void updateUI();      ///< 更新UI
    void onUpgradeClicked();   ///< 点击升级按钮
    void onCloseClicked();     ///< 点击关闭按钮
    void onTrainClicked();     ///< 点击训练按钮
    void onMoveClicked();      ///< 点击移动按钮
    void onArmyCampClicked();  ///< 点击军队详情按钮

    /**
     * @brief 获取资源类型名称
     * @param type 资源类型
     * @return std::string 资源名称
     */
    std::string getResourceTypeName(ResourceType type) const;

    /**
     * @brief 格式化时间
     * @param seconds 秒数
     * @return std::string 格式化后的时间字符串
     */
    std::string formatTime(int seconds) const;

private:
    BaseBuilding* _building = nullptr;              ///< 目标建筑
    cocos2d::ui::Layout* _panel = nullptr;          ///< 面板
    cocos2d::Label* _titleLabel = nullptr;          ///< 标题标签
    cocos2d::Label* _levelLabel = nullptr;          ///< 等级标签
    cocos2d::Label* _descLabel = nullptr;           ///< 描述标签
    cocos2d::Label* _costLabel = nullptr;           ///< 费用标签
    cocos2d::Label* _timeLabel = nullptr;           ///< 时间标签
    cocos2d::ui::Button* _upgradeButton = nullptr;  ///< 升级按钮
    cocos2d::ui::Button* _closeButton = nullptr;    ///< 关闭按钮
    cocos2d::ui::Button* _trainButton = nullptr;    ///< 训练按钮
    cocos2d::ui::Button* _moveButton = nullptr;     ///< 移动按钮
    cocos2d::ui::Button* _armyCampButton = nullptr; ///< 军队详情按钮

    UpgradeResultCallback _resultCallback = nullptr;  ///< 升级回调
    CloseCallback _closeCallback = nullptr;           ///< 关闭回调
};