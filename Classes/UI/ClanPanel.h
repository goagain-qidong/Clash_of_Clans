/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanPanel.h
 * File Function: 负责游戏部落面板
 * Author:        赵崇治
 * Update Date:   2025/12/16
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __CLAN_PANEL_H__
#define __CLAN_PANEL_H__

#include "Managers/SocketClient.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <map>
#include <set>
#include <string>
#include <vector>

// 玩家战斗状态
struct PlayerBattleStatus
{
    bool        isInBattle = false;
    std::string opponentId;
    std::string opponentName;
    bool        isAttacker = false; // true=攻击方, false=防守方
};

class ClanPanel : public cocos2d::Layer
{
public:
    enum class TabType
    {
        ONLINE_PLAYERS,
        CLAN_MEMBERS,
        CLAN_WAR
    };

    static ClanPanel* create();
    virtual bool      init() override;

    void show();
    void hide();

    virtual void onExit() override;

private:
    void setupUI();
    void setupConnectionUI();
    void setupMemberUI();
    void setupClanManagementUI();
    void updateUIState();
    void updateClanInfoDisplay();

    // 注册PVP和观战回调
    void registerPvpCallbacks();

    // 🆕 统一注册部落列表回调
    void registerClanListCallback();

    // Tab management
    void switchToTab(TabType tabType);
    void refreshCurrentTab();

    // 🆕 安全刷新，防止重入
    void safeRefreshCurrentTab();

    // 🆕 强制加入部落对话框
    void showJoinClanFirstDialog();

    // Request functions
    void requestOnlinePlayers();
    void requestClanMembers();
    void requestClanWarInfo();
    void requestBattleStatusList();
    void requestMyClanInfo();

    // Data received callbacks
    void onUserListReceived(const std::string& data);
    void onClanMembersReceived(const std::string& json);
    void onClanWarMemberListReceived(const std::string& json);
    void onBattleStatusReceived(const std::string& json);
    void onClanListReceived(const std::vector<ClanInfoClient>& clans);

    // Item creation
    void createOnlinePlayerItem(const std::string& userId, const std::string& username, int thLevel, int gold,
                                int elixir, const PlayerBattleStatus& battleStatus);
    void createMemberItem(const std::string& id, const std::string& name, int trophies, bool isOnline,
                          const PlayerBattleStatus& battleStatus);
    void createClanWarMemberItem(const std::string& userId, const std::string& username, int bestStars,
                                 float bestDestruction, bool canAttack, const PlayerBattleStatus& battleStatus);
    void createClanListItem(const std::string& clanId, const std::string& clanName, int memberCount, int clanTrophies,
                            int requiredTrophies, bool isOpen);

    // Button callbacks
    void onConnectClicked();
    void onAttackClicked(const std::string& memberId);
    void onSpectateClicked(const std::string& memberId);
    void onClanWarAttackClicked(const std::string& targetId);
    void onClanWarSpectateClicked(const std::string& targetId);

    // 部落管理回调
    void onCreateClanClicked();
    void onJoinClanClicked(const std::string& clanId);
    void onLeaveClanClicked();
    void showCreateClanDialog();
    void showClanListDialog();
    void showToast(const std::string& message, const cocos2d::Color4B& color = cocos2d::Color4B::WHITE);

    // 进入观战场景
    void enterSpectateScene(const std::string& attackerId, const std::string& defenderId, const std::string& mapData);

    // 进入战斗场景（作为攻击方）
    void enterBattleScene(const std::string& targetId, const std::string& mapData);

    cocos2d::Node* _panelNode          = nullptr;
    cocos2d::Node* _connectionNode     = nullptr;
    cocos2d::Node* _memberNode         = nullptr;
    cocos2d::Node* _clanManagementNode = nullptr;

    cocos2d::ui::TextField* _ipInput    = nullptr;
    cocos2d::ui::TextField* _portInput  = nullptr;
    cocos2d::ui::ListView*  _memberList = nullptr;

    // Tab buttons
    cocos2d::ui::Button* _onlinePlayersTab = nullptr;
    cocos2d::ui::Button* _clanMembersTab   = nullptr;
    cocos2d::ui::Button* _clanWarTab       = nullptr;

    // 部落管理UI控件
    cocos2d::Label*      _clanInfoLabel = nullptr;
    cocos2d::ui::Button* _createClanBtn = nullptr;
    cocos2d::ui::Button* _joinClanBtn   = nullptr;
    cocos2d::ui::Button* _leaveClanBtn  = nullptr;

    TabType _currentTab = TabType::ONLINE_PLAYERS;

    // 部落战状态
    std::string _currentWarId;

    // 玩家战斗状态缓存
    std::map<std::string, PlayerBattleStatus> _battleStatusCache;

    // 当前用户部落信息
    std::string _currentClanId;
    std::string _currentClanName;
    bool        _isInClan = false;

    // 缓存的部落列表
    std::vector<ClanInfoClient> _cachedClanList;

    // 正在进行战斗的玩家集合（用于显示"战斗中"标识）
    std::set<std::string> _playersInBattle;

    // 🆕 防止重入的刷新锁
    bool _isRefreshing = false;

    // 🆕 待加入的部落ID（用于处理异步回调）
    std::string _pendingClanId;

    // 定时刷新
    void scheduleRefresh();
    void unscheduleRefresh();
};

#endif // __CLAN_PANEL_H__