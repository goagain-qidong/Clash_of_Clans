/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleUI.h
 * File Function: 战斗界面 - 负责管理游戏中的战斗相关UI
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/

#pragma once
#ifndef __BATTLE_UI_H__
#define __BATTLE_UI_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Unit/unit.h" // For UnitType

class BattleUI : public cocos2d::Layer {
public:
    static BattleUI* create();
    virtual bool init() override;

    // Callbacks
    void setEndBattleCallback(const std::function<void()>& callback);
    void setReturnCallback(const std::function<void()>& callback);
    void setTroopSelectionCallback(const std::function<void(UnitType)>& callback);

    // Updates
    void updateStatus(const std::string& text, const cocos2d::Color4B& color);
    void updateTimer(int remainingTime);
    void updateStars(int stars);
    void updateDestruction(int percent);
    void updateTroopCounts(int barbarianCount, int archerCount, int giantCount);
    
    // Visibility & Mode
    void setReplayMode(bool isReplay);
    void showBattleHUD(bool visible); // Timer, stars, destruction, end button
    void showTroopButtons(bool visible);
    void showReturnButton(bool visible);
    void showResultPanel(int stars, int destructionPercent, int goldLooted, int elixirLooted, int trophyChange, bool isReplayMode);
    void highlightTroopButton(UnitType type);
    void setEndBattleButtonText(const std::string& text);

private:
    void setupTopBar();
    void setupBottomButtons();
    void setupTroopButtons();

    cocos2d::Size _visibleSize;
    bool _isReplayMode = false;
    int _starsEarned = 0;

    // UI Elements
    cocos2d::Label* _statusLabel = nullptr;
    cocos2d::Label* _timerLabel = nullptr;
    cocos2d::Label* _starsLabel = nullptr;
    cocos2d::Label* _destructionLabel = nullptr;
    cocos2d::ui::Button* _endBattleButton = nullptr;
    cocos2d::ui::Button* _returnButton = nullptr;
    
    // Troop Buttons
    cocos2d::ui::Button* _barbarianButton = nullptr;
    cocos2d::ui::Button* _archerButton = nullptr;
    cocos2d::ui::Button* _giantButton = nullptr;
    cocos2d::Label* _barbarianCountLabel = nullptr;
    cocos2d::Label* _archerCountLabel = nullptr;
    cocos2d::Label* _giantCountLabel = nullptr;

    // Callbacks
    std::function<void()> _onEndBattle;
    std::function<void()> _onReturn;
    std::function<void(UnitType)> _onTroopSelected;
};

#endif // __BATTLE_UI_H__