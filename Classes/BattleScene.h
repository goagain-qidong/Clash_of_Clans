#pragma once
#ifndef __BATTLE_SCENE_H__
#define __BATTLE_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Managers/SocketClient.h"
#include <string>

class BattleScene : public cocos2d::Scene {
public:
    static cocos2d::Scene* createScene();
    static BattleScene* create();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    // 设置对手信息
    void setOpponentInfo(const std::string& opponentId, int opponentTrophies);
    void setOpponentMap(const std::string& mapData);
    
private:
    BattleScene() = default;
    ~BattleScene() = default;
    
    enum class BattleState {
        MATCHING,       // 匹配中
        PREPARING,      // 准备战斗
        FIGHTING,       // 战斗中
        FINISHED        // 战斗结束
    };
    
    BattleState _state = BattleState::MATCHING;
    
    std::string _opponentId;
    int _opponentTrophies = 0;
    std::string _opponentMapData;
    
    float _battleTime = 180.0f;  // 3分钟战斗时间
    int _starsEarned = 0;
    int _goldLooted = 0;
    int _elixirLooted = 0;
    int _destructionPercent = 0;
    
    // UI 元素
    cocos2d::Label* _statusLabel;
    cocos2d::Label* _timerLabel;
    cocos2d::Label* _starsLabel;
    cocos2d::Label* _lootLabel;
    cocos2d::ui::Button* _cancelButton;
    cocos2d::ui::Button* _attackButton;
    cocos2d::ui::Button* _endButton;
    cocos2d::Node* _loadingNode;
    
    // 地图
    cocos2d::Sprite* _mapSprite;
    
    void setupUI();
    void setupCallbacks();
    
    void onMatchFound(const MatchInfo& matchInfo);
    void onAttackStart(const std::string& mapData);
    void onDisconnected();
    
    void startMatching();
    void cancelMatching();
    void startBattle();
    void endBattle();
    void returnToMain();
    
    void updateTimer(float dt);
    void showResult();
    
    // 战斗逻辑（简化版）
    void simulateBattle();
};

#endif // __BATTLE_SCENE_H__