/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SceneUIController.h
 * File Function: 场景UI控制器 - 负责管理游戏场景中的UI元素
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef __SCENE_UI_CONTROLLER_H__
#define __SCENE_UI_CONTROLLER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "BuildingData.h"

#include <functional>
#include <vector>

class GridMap;
class MapConfigManager;

/**
 * @class SceneUIController
 * @brief 场景UI控制器 - 负责所有UI的创建和管理
 *
 * 职责：
 * - 创建和管理场景按钮
 * - 管理建筑选择列表
 * - 管理确认/取消按钮
 * - 管理提示信息
 */
class SceneUIController : public cocos2d::Node
{
public:
    CREATE_FUNC(SceneUIController);

    virtual bool init() override;

    using ButtonCallback = std::function<void()>;
    using BuildingCallback = std::function<void(const BuildingData&)>;
    using MapChangedCallback = std::function<void(const std::string&)>;

    /** @brief 设置商店点击回调 */
    void setOnShopClicked(const ButtonCallback& callback) { _onShopClicked = callback; }

    /** @brief 设置攻击点击回调 */
    void setOnAttackClicked(const ButtonCallback& callback) { _onAttackClicked = callback; }

    /** @brief 设置部落点击回调 */
    void setOnClanClicked(const ButtonCallback& callback) { _onClanClicked = callback; }

    /** @brief 设置建筑选择回调 */
    void setOnBuildingSelected(const BuildingCallback& callback) { _onBuildingSelected = callback; }

    /** @brief 设置确认建造回调 */
    void setOnConfirmBuilding(const ButtonCallback& callback) { _onConfirmBuilding = callback; }

    /** @brief 设置取消建造回调 */
    void setOnCancelBuilding(const ButtonCallback& callback) { _onCancelBuilding = callback; }

    /** @brief 设置账号切换回调 */
    void setOnAccountSwitched(const ButtonCallback& callback) { _onAccountSwitched = callback; }

    /** @brief 设置登出回调 */
    void setOnLogout(const ButtonCallback& callback) { _onLogout = callback; }

    /** @brief 设置地图切换回调 */
    void setOnMapChanged(const MapChangedCallback& callback) { _onMapChanged = callback; }

    /** @brief 设置防御日志点击回调 */
    void setOnDefenseLogClicked(const ButtonCallback& callback) { _onDefenseLogClicked = callback; }

    /**
     * @brief 设置建筑列表数据
     * @param buildings 建筑数据列表
     */
    void setBuildingList(const std::vector<BuildingData>& buildings);

    /** @brief 切换建筑选择列表显示 */
    void toggleBuildingList();

    /**
     * @brief 显示建筑放置确认按钮
     * @param worldPos 建筑的世界坐标
     */
    void showConfirmButtons(const cocos2d::Vec2& worldPos);

    /** @brief 隐藏确认按钮 */
    void hideConfirmButtons();

    /**
     * @brief 显示提示信息
     * @param hint 提示文本
     */
    void showHint(const std::string& hint);

    /** @brief 隐藏提示信息 */
    void hideHint();

    void setShopButtonVisible(bool visible);
    void setAttackButtonVisible(bool visible);
    void setClanButtonVisible(bool visible);

private:
    cocos2d::Size _visibleSize;  ///< 可视区域大小

    cocos2d::ui::Button* _shopButton = nullptr;       ///< 商店按钮
    cocos2d::ui::Button* _attackButton = nullptr;     ///< 攻击按钮
    cocos2d::ui::Button* _clanButton = nullptr;       ///< 部落按钮
    cocos2d::ui::Button* _settingsButton = nullptr;   ///< 设置按钮
    cocos2d::ui::Button* _defenseLogButton = nullptr; ///< 防御日志按钮

    cocos2d::ui::ListView* _buildingListUI = nullptr;  ///< 建筑列表UI

    cocos2d::ui::Button* _confirmButton = nullptr;  ///< 确认按钮
    cocos2d::ui::Button* _cancelButton = nullptr;   ///< 取消按钮

    cocos2d::Label* _hintLabel = nullptr;  ///< 提示标签

    bool _isBuildingListVisible = false;  ///< 建筑列表是否可见

    ButtonCallback _onShopClicked;         ///< 商店点击回调
    ButtonCallback _onAttackClicked;       ///< 攻击点击回调
    ButtonCallback _onClanClicked;         ///< 部落点击回调
    ButtonCallback _onAccountSwitched;     ///< 账号切换回调
    ButtonCallback _onLogout;              ///< 登出回调
    MapChangedCallback _onMapChanged;      ///< 地图切换回调
    BuildingCallback _onBuildingSelected;  ///< 建筑选择回调
    ButtonCallback _onConfirmBuilding;     ///< 确认建造回调
    ButtonCallback _onCancelBuilding;      ///< 取消建造回调
    ButtonCallback _onDefenseLogClicked;   ///< 防御日志点击回调

    std::vector<BuildingData> _buildingList;  ///< 建筑列表数据

    void setupMainButtons();
    void createBuildingListUI();
    void onSettingsClicked();
    cocos2d::ui::Button* createFlatButton(const std::string& text, const cocos2d::Size& size, const cocos2d::Color3B& color, const std::function<void(cocos2d::Ref*)>& callback);
};

#endif // __SCENE_UI_CONTROLLER_H__
