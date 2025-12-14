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
#include <functional>
#include <vector>
#include "BuildingData.h"

// 前向声明
class GridMap;
class MapConfigManager;

/**
 * @class SceneUIController
 * @brief 场景UI控制器 - 负责所有UI的创建和管理
 * 
 * 职责：
 * - 创建和管理场景按钮（Shop、Map、Attack、Settings等）
 * - 管理建筑选择列表
 * - 管理确认/取消按钮
 * - 管理提示信息
 */
class SceneUIController : public cocos2d::Node
{
public:
    CREATE_FUNC(SceneUIController);
    
    virtual bool init() override;
    
    // ==================== 回调设置 ====================
    
    using ButtonCallback = std::function<void()>;
    using BuildingCallback = std::function<void(const BuildingData&)>;
    using MapChangedCallback = std::function<void(const std::string&)>;
    
    void setOnShopClicked(const ButtonCallback& callback) { _onShopClicked = callback; }
    void setOnAttackClicked(const ButtonCallback& callback) { _onAttackClicked = callback; }
    void setOnClanClicked(const ButtonCallback& callback) { _onClanClicked = callback; }
    void setOnBuildingSelected(const BuildingCallback& callback) { _onBuildingSelected = callback; }
    void setOnConfirmBuilding(const ButtonCallback& callback) { _onConfirmBuilding = callback; }
    void setOnCancelBuilding(const ButtonCallback& callback) { _onCancelBuilding = callback; }
    void setOnAccountSwitched(const ButtonCallback& callback) { _onAccountSwitched = callback; }
    void setOnLogout(const ButtonCallback& callback) { _onLogout = callback; }
    void setOnMapChanged(const MapChangedCallback& callback) { _onMapChanged = callback; }
    void setOnDefenseLogClicked(const ButtonCallback& callback) { _onDefenseLogClicked = callback; } // 🆕 新增
    
    // ==================== 建筑列表 ====================
    
    /**
     * @brief 设置建筑列表数据
     */
    void setBuildingList(const std::vector<BuildingData>& buildings);
    
    /**
     * @brief 显示/隐藏建筑选择列表
     */
    void toggleBuildingList();
    
    // ==================== 确认按钮 ====================
    
    /**
     * @brief 显示建筑放置确认按钮
     * @param worldPos 建筑的世界坐标
     */
    void showConfirmButtons(const cocos2d::Vec2& worldPos);
    
    /**
     * @brief 隐藏确认按钮
     */
    void hideConfirmButtons();
    
    // ==================== 提示信息 ====================
    
    /**
     * @brief 显示提示信息
     * @param hint 提示文本
     */
    void showHint(const std::string& hint);
    
    /**
     * @brief 隐藏提示信息
     */
    void hideHint();
    
    // ==================== 按钮可见性 ====================
    
    void setShopButtonVisible(bool visible);
    void setAttackButtonVisible(bool visible);
    void setClanButtonVisible(bool visible);
    
private:
    cocos2d::Size _visibleSize;
    
    // ==================== UI元素 ====================
    cocos2d::ui::Button* _shopButton = nullptr;
    cocos2d::ui::Button* _attackButton = nullptr;
    cocos2d::ui::Button* _clanButton = nullptr;
    cocos2d::ui::Button* _settingsButton = nullptr;
    cocos2d::ui::Button* _defenseLogButton = nullptr; // 🆕 新增
    
    cocos2d::ui::ListView* _buildingListUI = nullptr;
    
    cocos2d::ui::Button* _confirmButton = nullptr;
    cocos2d::ui::Button* _cancelButton = nullptr;
    
    cocos2d::Label* _hintLabel = nullptr;
    
    bool _isBuildingListVisible = false;
    
    // ==================== 回调 ====================
    ButtonCallback _onShopClicked;
    ButtonCallback _onAttackClicked;
    ButtonCallback _onClanClicked;
    ButtonCallback _onAccountSwitched;
    ButtonCallback _onLogout;
    MapChangedCallback _onMapChanged;
    BuildingCallback _onBuildingSelected;
    ButtonCallback _onConfirmBuilding;
    ButtonCallback _onCancelBuilding;
    ButtonCallback _onDefenseLogClicked; // 🆕 新增
    
    // ==================== 数据 ====================
    std::vector<BuildingData> _buildingList;
    
    // ==================== 内部方法 ====================
    void setupMainButtons();
    void createBuildingListUI();
    void onSettingsClicked();
    
    // 🆕 辅助方法：创建扁平化按钮
    cocos2d::ui::Button* createFlatButton(const std::string& text, const cocos2d::Size& size, const cocos2d::Color3B& color, const std::function<void(cocos2d::Ref*)>& callback);
};

#endif // __SCENE_UI_CONTROLLER_H__
