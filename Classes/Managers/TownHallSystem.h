/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TownHallSystem.h
 * File Function: 大本营系统
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include "Managers/ResourceManager.h"
#include "ui/CocosGUI.h"

#include <functional>
#include <map>
#include <string>

class TownHallBuilding;

/**
 * @class TownHallUpgradeUI
 * @brief 大本营升级UI
 */
class TownHallUpgradeUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建升级UI
     * @param building 大本营建筑
     * @return TownHallUpgradeUI* UI指针
     */
    static TownHallUpgradeUI* create(TownHallBuilding* building);

    /**
     * @brief 初始化
     * @param building 大本营建筑
     * @return bool 是否成功
     */
    virtual bool init(TownHallBuilding* building);

    /** @brief 显示UI */
    void show();

    /** @brief 隐藏UI */
    void hide();

    /** @brief 是否可见 */
    bool isVisible() const;

    /**
     * @brief 设置位置到建筑附近
     * @param building 建筑节点
     */
    void setPositionNearBuilding(cocos2d::Node* building);

    using UpgradeCallback = std::function<void(bool success, int newLevel)>;

    /** @brief 设置升级回调 */
    void setUpgradeCallback(const UpgradeCallback& callback) { _upgradeCallback = callback; }

private:
    TownHallBuilding* _building;               ///< 大本营建筑
    cocos2d::ui::Button* _upgradeButton;       ///< 升级按钮
    cocos2d::Label* _infoLabel;                ///< 信息标签
    UpgradeCallback _upgradeCallback;          ///< 升级回调

    void setupUI();                            ///< 设置UI
    void onUpgradeClicked(cocos2d::Ref* sender);  ///< 点击升级
    void updateInfo();                         ///< 更新信息
};

/**
 * @class ResourceDisplayUI
 * @brief 资源显示UI
 */
class ResourceDisplayUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建资源显示UI
     * @return ResourceDisplayUI* UI指针
     */
    static ResourceDisplayUI* create();

    virtual bool init() override;

    /** @brief 更新显示 */
    void updateDisplay();

    /** @brief 设置位置到左上角 */
    void setPositionAtTopLeft();

    /** @brief 设置位置到右上角 */
    void setPositionAtTopRight();

    /**
     * @brief 设置自定义位置
     * @param position 位置
     */
    void setCustomPosition(const cocos2d::Vec2& position);

    /**
     * @brief 显示/隐藏资源
     * @param type 资源类型
     * @param show 是否显示
     */
    void showResource(ResourceType type, bool show);

private:
    /**
     * @struct ResourceDisplay
     * @brief 资源显示数据
     */
    struct ResourceDisplay
    {
        cocos2d::Label* icon;        ///< 图标
        cocos2d::Label* amount;      ///< 数量
        cocos2d::Node* container;    ///< 容器
    };

    std::map<ResourceType, ResourceDisplay> _displays;  ///< 显示映射

    /**
     * @brief 设置资源
     * @param type 资源类型
     * @param icon 图标
     * @param color 颜色
     */
    void setupResource(ResourceType type, const std::string& icon, const cocos2d::Color4B& color);
};