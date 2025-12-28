/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanDataCache.h
 * File Function: 部落数据缓存 - 统一管理部落相关数据
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef __CLAN_DATA_CACHE_H__
#define __CLAN_DATA_CACHE_H__

#include "Managers/SocketClient.h"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

/**
 * @struct PlayerBattleStatus
 * @brief 玩家战斗状态
 */
struct PlayerBattleStatus
{
    bool isInBattle = false;      ///< 是否在战斗中
    std::string opponentId;       ///< 对手ID
    std::string opponentName;     ///< 对手名称
    bool isAttacker = false;      ///< 是否为攻击方
};

/**
 * @struct OnlinePlayerInfo
 * @brief 在线玩家信息
 */
struct OnlinePlayerInfo
{
    std::string userId;    ///< 用户ID
    std::string username;  ///< 用户名
    int thLevel = 1;       ///< 大本营等级
    int gold = 0;          ///< 金币
    int elixir = 0;        ///< 圣水
};

/**
 * @struct ClanMemberInfo
 * @brief 部落成员信息
 */
struct ClanMemberInfo
{
    std::string id;        ///< 成员ID
    std::string name;      ///< 成员名称
    int trophies = 0;      ///< 奖杯数
    bool isOnline = false; ///< 是否在线
};

/**
 * @struct ClanWarMemberInfo
 * @brief 部落战成员信息
 */
struct ClanWarMemberInfo
{
    std::string userId;           ///< 用户ID
    std::string username;         ///< 用户名
    int bestStars = 0;            ///< 最佳星星数
    float bestDestruction = 0.0f; ///< 最佳摧毁率
    bool canAttack = false;       ///< 是否可以攻击
};

/**
 * @enum ClanDataChangeType
 * @brief 数据变更通知类型
 */
enum class ClanDataChangeType
{
    ONLINE_PLAYERS,    ///< 在线玩家
    CLAN_MEMBERS,      ///< 部落成员
    CLAN_WAR_MEMBERS,  ///< 部落战成员
    BATTLE_STATUS,     ///< 战斗状态
    CLAN_LIST,         ///< 部落列表
    CLAN_INFO,         ///< 部落信息
    CHAT_MESSAGE       ///< 聊天消息
};

/**
 * @class ClanDataCache
 * @brief 部落数据缓存（单例）
 */
class ClanDataCache
{
public:
    /**
     * @brief 获取单例实例
     * @return ClanDataCache& 单例引用
     */
    static ClanDataCache& getInstance();

    ClanDataCache(const ClanDataCache&) = delete;
    ClanDataCache& operator=(const ClanDataCache&) = delete;

    /** @brief 获取在线玩家列表 */
    const std::vector<OnlinePlayerInfo>& getOnlinePlayers() const { return _onlinePlayers; }

    /** @brief 获取部落成员列表 */
    const std::vector<ClanMemberInfo>& getClanMembers() const { return _clanMembers; }

    /** @brief 获取部落战成员列表 */
    const std::vector<ClanWarMemberInfo>& getClanWarMembers() const { return _clanWarMembers; }

    /** @brief 获取部落列表 */
    const std::vector<ClanInfoClient>& getClanList() const { return _clanList; }

    /**
     * @brief 获取玩家战斗状态
     * @param playerId 玩家ID
     * @return const PlayerBattleStatus& 战斗状态
     */
    const PlayerBattleStatus& getBattleStatus(const std::string& playerId) const;

    /**
     * @brief 检查玩家是否在战斗中
     * @param playerId 玩家ID
     * @return bool 是否在战斗中
     */
    bool isPlayerInBattle(const std::string& playerId) const;

    /** @brief 获取当前部落ID */
    const std::string& getCurrentClanId() const { return _currentClanId; }

    /** @brief 获取当前部落名称 */
    const std::string& getCurrentClanName() const { return _currentClanName; }

    /** @brief 是否在部落中 */
    bool isInClan() const { return _isInClan; }

    /** @brief 获取当前战争ID */
    const std::string& getCurrentWarId() const { return _currentWarId; }

    void setOnlinePlayers(const std::vector<OnlinePlayerInfo>& players);
    void setClanMembers(const std::vector<ClanMemberInfo>& members);
    void setClanWarMembers(const std::vector<ClanWarMemberInfo>& members);
    void setClanList(const std::vector<ClanInfoClient>& clans);
    void setBattleStatusMap(const std::map<std::string, PlayerBattleStatus>& statusMap);
    void setCurrentClan(const std::string& clanId, const std::string& clanName);
    void clearCurrentClan();
    void setCurrentWarId(const std::string& warId) { _currentWarId = warId; }

    /**
     * @brief 根据ID查找部落名称
     * @param clanId 部落ID
     * @return std::string 部落名称
     */
    std::string findClanNameById(const std::string& clanId) const;

    /**
     * @struct ChatMessage
     * @brief 聊天消息
     */
    struct ChatMessage
    {
        std::string sender;
        std::string message;
    };

    /** @brief 添加聊天消息（返回是否为新消息） */
    bool addChatMessage(const std::string& sender, const std::string& message);

    /** @brief 获取聊天记录 */
    const std::vector<ChatMessage>& getChatHistory() const { return _chatHistory; }

    /** @brief 保存聊天记录到本地 */
    void saveChatHistory();

    /** @brief 从本地加载聊天记录 */
    void loadChatHistory();

    /** @brief 清空聊天记录 */
    void clearChatHistory();

    using DataChangeCallback = std::function<void(ClanDataChangeType)>;

    /**
     * @brief 添加观察者
     * @param owner 所有者指针
     * @param callback 回调函数
     */
    void addObserver(void* owner, DataChangeCallback callback);

    /**
     * @brief 移除观察者
     * @param owner 所有者指针
     */
    void removeObserver(void* owner);

private:
    ClanDataCache() = default;

    void notifyObservers(ClanDataChangeType type);

    std::vector<OnlinePlayerInfo> _onlinePlayers;       ///< 在线玩家列表
    std::vector<ClanMemberInfo> _clanMembers;           ///< 部落成员列表
    std::vector<ClanWarMemberInfo> _clanWarMembers;     ///< 部落战成员列表
    std::vector<ClanInfoClient> _clanList;              ///< 部落列表
    std::vector<ChatMessage> _chatHistory;              ///< 聊天记录
    std::map<std::string, PlayerBattleStatus> _battleStatusMap;  ///< 战斗状态映射
    std::set<std::string> _playersInBattle;             ///< 战斗中的玩家

    std::string _currentClanId;    ///< 当前部落ID
    std::string _currentClanName;  ///< 当前部落名称
    bool _isInClan = false;        ///< 是否在部落中
    std::string _currentWarId;     ///< 当前战争ID

    std::map<void*, DataChangeCallback> _observers;  ///< 观察者列表

    static const PlayerBattleStatus _emptyStatus;  ///< 空状态
};

#endif // __CLAN_DATA_CACHE_H__