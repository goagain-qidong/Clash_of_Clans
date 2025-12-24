/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     PlayerListItem.h
 * File Function: 玩家列表项组件 - 可复用的列表项UI
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef __PLAYER_LIST_ITEM_WIDGET_H__
#define __PLAYER_LIST_ITEM_WIDGET_H__

#include "ClanDataCache.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include <functional>

/**
 * @class PlayerListItemWidget
 * @brief 玩家列表项组件 - 可复用的列表项UI
 */
class PlayerListItemWidget : public cocos2d::ui::Layout
{
public:
    using ActionCallback = std::function<void(const std::string& playerId)>;

    /**
     * @brief 创建在线玩家项
     * @param info 玩家信息
     * @param status 战斗状态
     * @param onAttack 攻击回调
     * @param onSpectate 观战回调
     * @return PlayerListItemWidget* 列表项
     */
    static PlayerListItemWidget* createOnlinePlayer(const OnlinePlayerInfo& info, const PlayerBattleStatus& status,
                                                    ActionCallback onAttack, ActionCallback onSpectate);

    /**
     * @brief 创建部落成员项
     * @param info 成员信息
     * @param status 战斗状态
     * @param onAttack 攻击回调
     * @param onSpectate 观战回调
     * @return PlayerListItemWidget* 列表项
     */
    static PlayerListItemWidget* createClanMember(const ClanMemberInfo& info, const PlayerBattleStatus& status,
                                                  ActionCallback onAttack, ActionCallback onSpectate);

    /**
     * @brief 创建部落战成员项
     * @param info 成员信息
     * @param status 战斗状态
     * @param onAttack 攻击回调
     * @param onSpectate 观战回调
     * @return PlayerListItemWidget* 列表项
     */
    static PlayerListItemWidget* createClanWarMember(const ClanWarMemberInfo& info, const PlayerBattleStatus& status,
                                                     ActionCallback onAttack, ActionCallback onSpectate);

private:
    bool initOnlinePlayer(const OnlinePlayerInfo& info, const PlayerBattleStatus& status, ActionCallback onAttack,
                          ActionCallback onSpectate);

    bool initClanMember(const ClanMemberInfo& info, const PlayerBattleStatus& status, ActionCallback onAttack,
                        ActionCallback onSpectate);

    bool initClanWarMember(const ClanWarMemberInfo& info, const PlayerBattleStatus& status, ActionCallback onAttack,
                           ActionCallback onSpectate);

    void addBattleStatusLabel(const PlayerBattleStatus& status, float x, float y);
    void addActionButtons(const std::string& playerId, const PlayerBattleStatus& status, bool canAttack,
                          ActionCallback onAttack, ActionCallback onSpectate);
};

#endif // __PLAYER_LIST_ITEM_WIDGET_H__