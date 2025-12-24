/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanPanel.h
 * File Function: 部落面板主容器 - 负责UI布局和协调
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

/**
 * @class ClanPanel
 * @brief 部落面板 - 管理部落相关UI
 */
class ClanPanel : public cocos2d::Layer
{
public:
    /**
     * @enum TabType
     * @brief 标签页类型
     */
    enum class TabType
    {
        ONLINE_PLAYERS,  ///< 在线玩家
        CLAN_MEMBERS,    ///< 部落成员
        CLAN_WAR         ///< 部落战
    };

    /**
     * @brief 创建面板
     * @return ClanPanel* 面板指针
     */
    static ClanPanel* create();

    virtual bool init() override;
    virtual void onExit() override;

    /** @brief 显示面板 */
    void show();

    /** @brief 隐藏面板 */
    void hide();

private:
    void setupUI();             ///< 设置UI
    void setupConnectionUI();   ///< 设置连接UI
    void setupTabBar();         ///< 设置标签栏
    void setupListView();       ///< 设置列表视图
    void setupClanManagement(); ///< 设置部落管理

    void onDataChanged(ClanDataChangeType type);  ///< 数据变更回调

    void switchToTab(TabType tab);     ///< 切换标签页
    void refreshCurrentTab();          ///< 刷新当前标签页
    void safeRefreshCurrentTab();      ///< 安全刷新

    void renderOnlinePlayers();   ///< 渲染在线玩家
    void renderClanMembers();     ///< 渲染部落成员
    void renderClanWarMembers();  ///< 渲染部落战成员
    void renderEmptyState(const std::string& message, const cocos2d::Color4B& color = cocos2d::Color4B::GRAY);

    void updateClanInfoDisplay();      ///< 更新部落信息显示
    void showCreateClanDialog();       ///< 显示创建部落对话框
    void showClanListDialog();         ///< 显示部落列表对话框
    void showJoinClanFirstDialog();    ///< 显示加入部落提示
    void showLeaveClanConfirmDialog(); ///< 显示退出确认对话框

    void registerPvpCallbacks();     ///< 注册PVP回调
    void registerClanWarCallbacks(); ///< 注册部落战回调

    void onConnectClicked();  ///< 连接点击
    void onAttackPlayer(const std::string& playerId);   ///< 攻击玩家
    void onSpectatePlayer(const std::string& playerId); ///< 观战玩家
    void onClanWarAttack(const std::string& targetId);  ///< 部落战攻击
    void onClanWarSpectate(const std::string& targetId); ///< 部落战观战
    void onJoinClanClicked(const std::string& clanId);  ///< 加入部落
    void onLeaveClanClicked();  ///< 退出部落

    void enterBattleScene(const std::string& targetId, const std::string& mapData);
    void enterSpectateScene(const std::string& attackerId, const std::string& defenderId, const std::string& mapData);

    void showToast(const std::string& msg, const cocos2d::Color4B& color = cocos2d::Color4B::WHITE);
    void scheduleRefresh();    ///< 调度刷新
    void unscheduleRefresh();  ///< 取消调度刷新

    cocos2d::Node* _panelNode = nullptr;           ///< 面板节点
    cocos2d::Node* _connectionNode = nullptr;      ///< 连接节点
    cocos2d::Node* _memberNode = nullptr;          ///< 成员节点
    cocos2d::Node* _clanManagementNode = nullptr;  ///< 部落管理节点
    cocos2d::ui::TextField* _ipInput = nullptr;    ///< IP输入框
    cocos2d::ui::TextField* _portInput = nullptr;  ///< 端口输入框
    cocos2d::ui::ListView* _memberList = nullptr;  ///< 成员列表
    cocos2d::ui::Button* _onlinePlayersTab = nullptr;  ///< 在线玩家标签
    cocos2d::ui::Button* _clanMembersTab = nullptr;    ///< 部落成员标签
    cocos2d::Label* _clanInfoLabel = nullptr;      ///< 部落信息标签
    cocos2d::ui::Button* _createClanBtn = nullptr; ///< 创建部落按钮
    cocos2d::ui::Button* _joinClanBtn = nullptr;   ///< 加入部落按钮
    cocos2d::ui::Button* _leaveClanBtn = nullptr;  ///< 退出部落按钮

    TabType _currentTab = TabType::ONLINE_PLAYERS;  ///< 当前标签页
    bool _isRefreshing = false;   ///< 是否正在刷新
    std::string _currentWarId;    ///< 当前战争ID
};

#endif // __CLAN_PANEL_H__