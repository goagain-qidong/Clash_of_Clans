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
    static ClanPanel* create();
    virtual bool init() override;
    
    void show();
    void hide();

private:
    void setupUI();
    void setupConnectionUI(); // 🆕 Connection UI
    void setupMemberUI();     // 🆕 Member List UI
    void updateUIState();     // 🆕 Switch between connection and member list

    void requestClanMembers();
    void onClanMembersReceived(const std::string& json);
    void createMemberItem(const std::string& id, const std::string& name, int trophies, bool isOnline);
    
    // Callbacks
    void onConnectClicked(); // 🆕 Connect button callback
    void onAttackClicked(const std::string& memberId);
    void onSpectateClicked(const std::string& memberId);
    
    cocos2d::Node* _panelNode = nullptr;
    cocos2d::Node* _connectionNode = nullptr; // 🆕
    cocos2d::Node* _memberNode = nullptr;     // 🆕
    
    cocos2d::ui::TextField* _ipInput = nullptr;   // 🆕
    cocos2d::ui::TextField* _portInput = nullptr; // 🆕
    cocos2d::ui::ListView* _memberList = nullptr;
};

#endif // __CLAN_PANEL_H__