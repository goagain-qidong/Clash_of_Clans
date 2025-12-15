/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanPanel.h
 * File Function: 负责游戏部落面板
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __CLAN_PANEL_H__
#define __CLAN_PANEL_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <string>
#include <vector>

class ClanPanel : public cocos2d::Layer {
public:
    enum class TabType {
        ONLINE_PLAYERS,
        CLAN_MEMBERS,
        CLAN_WAR
    };

    static ClanPanel* create();
    virtual bool init() override;
    
    void show();
    void hide();

    virtual void onExit() override;

private:
    void setupUI();
    void setupConnectionUI();
    void setupMemberUI();
    void updateUIState();

    // Tab management
    void switchToTab(TabType tabType);
    void refreshCurrentTab();
    
    // Request functions
    void requestOnlinePlayers();
    void requestClanMembers();
    void requestClanWarInfo();
    
    // Data received callbacks
    void onUserListReceived(const std::string& data);
    void onClanMembersReceived(const std::string& json);
    
    // Item creation
    void createOnlinePlayerItem(const std::string& userId, const std::string& username, int thLevel, int gold, int elixir);
    void createMemberItem(const std::string& id, const std::string& name, int trophies, bool isOnline);
    
    // Button callbacks
    void onConnectClicked();
    void onAttackClicked(const std::string& memberId);
    void onSpectateClicked(const std::string& memberId);
    
    cocos2d::Node* _panelNode = nullptr;
    cocos2d::Node* _connectionNode = nullptr;
    cocos2d::Node* _memberNode = nullptr;
    
    cocos2d::ui::TextField* _ipInput = nullptr;
    cocos2d::ui::TextField* _portInput = nullptr;
    cocos2d::ui::ListView* _memberList = nullptr;
    
    // Tab buttons
    cocos2d::ui::Button* _onlinePlayersTab = nullptr;
    cocos2d::ui::Button* _clanMembersTab = nullptr;
    cocos2d::ui::Button* _clanWarTab = nullptr;
    
    TabType _currentTab = TabType::ONLINE_PLAYERS;
};

#endif // __CLAN_PANEL_H__