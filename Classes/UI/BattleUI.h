/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleUI.h
 * File Function: 战斗界面 - 负责管理游戏中的战斗相关UI
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef BATTLE_UI_H_
#define BATTLE_UI_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "Unit/UnitTypes.h"

class BattleUI : public cocos2d::Layer
{
public:
    static BattleUI* create();
    virtual bool     init() override;

    // Callbacks
    void setEndBattleCallback(const std::function<void()>& callback);
    void setReturnCallback(const std::function<void()>& callback);
    void setTroopSelectionCallback(const std::function<void(UnitType)>& callback);
    void setTroopDeselectionCallback(const std::function<void()>& callback);

    // Updates
    void updateStatus(const std::string& text, const cocos2d::Color4B& color);
    void updateTimer(int remainingTime);
    void updateStars(int stars);
    void updateDestruction(int percent);
    void updateTroopCounts(int barbarianCount, int archerCount, int giantCount, int goblinCount, int wallBreakerCount);

    // Visibility & Mode
    void setReplayMode(bool isReplay);
    void showBattleHUD(bool visible); // Timer, stars, destruction, end button
    void showTroopButtons(bool visible);
    void showReturnButton(bool visible);
    void showResultPanel(int stars, int destructionPercent, int goldLooted, int elixirLooted, int trophyChange,
                         bool isReplayMode);
    void highlightTroopButton(UnitType type);
    // 🆕 取消所有高亮
    void clearTroopHighlight();
    void setEndBattleButtonText(const std::string& text);

    // 🆕 获取当前选中的兵种（用于判断是否已选中）
    UnitType getSelectedUnitType() const { return _selectedUnitType; }
    bool     hasSelectedUnit() const { return _hasSelectedUnit; }

private:
    void setupTopBar();
    void setupBottomButtons();
    void setupTroopButtons();
    // 🆕 创建单个兵种卡片
    cocos2d::Node* createTroopCard(UnitType type, const std::string& iconPath, const std::string& name);
    // 🆕 更新单个卡片的数量显示
    void updateTroopCardCount(UnitType type, int count);
    // 🆕 处理兵种点击
    void onTroopCardClicked(UnitType type);

    cocos2d::Size _visibleSize;
    bool          _isReplayMode = false;
    int           _starsEarned  = 0;

    // 🆕 选中状态
    UnitType _selectedUnitType = UnitType::kBarbarian;
    bool     _hasSelectedUnit  = false;

    // UI Elements
    cocos2d::Label*      _statusLabel      = nullptr;
    cocos2d::Label*      _timerLabel       = nullptr;
    cocos2d::Label*      _starsLabel       = nullptr;
    cocos2d::Label*      _destructionLabel = nullptr;
    cocos2d::ui::Button* _endBattleButton  = nullptr;
    cocos2d::ui::Button* _returnButton     = nullptr;

    // 🆕 兵种卡片面板
    cocos2d::Node* _troopPanel = nullptr;

    // 🆕 兵种卡片（替代原来的按钮）
    cocos2d::Node* _barbarianCard   = nullptr;
    cocos2d::Node* _archerCard      = nullptr;
    cocos2d::Node* _giantCard       = nullptr;
    cocos2d::Node* _goblinCard      = nullptr;
    cocos2d::Node* _wallBreakerCard = nullptr;

    // 🆕 选中框精灵
    cocos2d::Sprite* _selectionFrame = nullptr;

    // Troop Buttons (保留旧的成员变量以保持兼容性)
    cocos2d::ui::Button* _barbarianButton       = nullptr;
    cocos2d::ui::Button* _archerButton          = nullptr;
    cocos2d::ui::Button* _giantButton           = nullptr;
    cocos2d::ui::Button* _goblinButton          = nullptr;
    cocos2d::ui::Button* _wallBreakerButton     = nullptr;
    cocos2d::Label*      _barbarianCountLabel   = nullptr;
    cocos2d::Label*      _archerCountLabel      = nullptr;
    cocos2d::Label*      _giantCountLabel       = nullptr;
    cocos2d::Label*      _goblinCountLabel      = nullptr;
    cocos2d::Label*      _wallBreakerCountLabel = nullptr;

    // Callbacks
    std::function<void()>         _onEndBattle;
    std::function<void()>         _onReturn;
    std::function<void(UnitType)> _onTroopSelected;
    std::function<void()>         _onTroopDeselected;
};

#endif // BATTLE_UI_H_