/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanPanel.h
 * File Function: 部落面板主容器 - 负责UI布局和协调（重构版）
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __CLAN_PANEL_H__
#define __CLAN_PANEL_H__

#include "ClanDataCache.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <string>

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
    virtual void      onExit() override;

    void show();
    void hide();

private:
    // ========== UI 初始化 ==========
    void setupUI();
    void setupConnectionUI();
    void setupTabBar();
    void setupListView();
    void setupClanManagement();

    // ========== 数据变更响应（观察者回调） ==========
    void onDataChanged(ClanDataChangeType type);

    // ========== Tab 切换与刷新 ==========
    void switchToTab(TabType tab);
    void refreshCurrentTab();
    void safeRefreshCurrentTab();

    // ========== 列表渲染 ==========
    void renderOnlinePlayers();
    void renderClanMembers();
    void renderClanWarMembers();
    void renderEmptyState(const std::string& message, const cocos2d::Color4B& color = cocos2d::Color4B::GRAY);

    // ========== 部落管理 ==========
    void updateClanInfoDisplay();
    void showCreateClanDialog();
    void showClanListDialog();
    void showJoinClanFirstDialog();
    void showLeaveClanConfirmDialog(); // 🆕 退出确认对话框

    // ========== 网络回调注册 ==========
    void registerPvpCallbacks();
    void registerClanWarCallbacks();

    // ========== 动作处理 ==========
    void onConnectClicked();
    void onAttackPlayer(const std::string& playerId);
    void onSpectatePlayer(const std::string& playerId);
    void onClanWarAttack(const std::string& targetId);
    void onClanWarSpectate(const std::string& targetId);
    void onJoinClanClicked(const std::string& clanId);
    void onLeaveClanClicked(); // 🆕 退出部落

    // ========== 场景切换 ==========
    void enterBattleScene(const std::string& targetId, const std::string& mapData);
    void enterSpectateScene(const std::string& attackerId, const std::string& defenderId, const std::string& mapData, const std::vector<std::string>& history = {});

    // ========== 辅助方法 ==========
    void showToast(const std::string& msg, const cocos2d::Color4B& color = cocos2d::Color4B::WHITE);
    void scheduleRefresh();
    void unscheduleRefresh();

    // ========== UI 控件 ==========
    cocos2d::Node*          _panelNode          = nullptr;
    cocos2d::Node*          _connectionNode     = nullptr;
    cocos2d::Node*          _memberNode         = nullptr;
    cocos2d::Node*          _clanManagementNode = nullptr;
    cocos2d::ui::TextField* _ipInput            = nullptr;
    cocos2d::ui::TextField* _portInput          = nullptr;
    cocos2d::ui::ListView*  _memberList         = nullptr;
    cocos2d::ui::Button*    _onlinePlayersTab   = nullptr;
    cocos2d::ui::Button*    _clanMembersTab     = nullptr;
    cocos2d::Label*         _clanInfoLabel      = nullptr;
    cocos2d::ui::Button*    _createClanBtn      = nullptr;
    cocos2d::ui::Button*    _joinClanBtn        = nullptr;
    cocos2d::ui::Button*    _leaveClanBtn       = nullptr;

    // ========== 状态 ==========
    TabType     _currentTab   = TabType::ONLINE_PLAYERS;
    bool        _isRefreshing = false;
    std::string _currentWarId;
    bool        _isTransitioningToBattle = false; // 🆕 Flag to prevent clearing callbacks on transition
};

#endif // __CLAN_PANEL_H__