/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanService.h
 * File Function: 部落服务层 - 处理网络通信和业务逻辑
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef __CLAN_SERVICE_H__
#define __CLAN_SERVICE_H__

#include "ClanDataCache.h"

#include <functional>
#include <string>

using OperationCallback = std::function<void(bool success, const std::string& message)>;

/**
 * @class ClanService
 * @brief 部落服务层（单例）- 处理网络通信和业务逻辑
 */
class ClanService
{
public:
    /**
     * @brief 获取单例实例
     * @return ClanService& 单例引用
     */
    static ClanService& getInstance();

    ClanService(const ClanService&) = delete;
    ClanService& operator=(const ClanService&) = delete;

    /**
     * @brief 连接服务器
     * @param ip 服务器IP
     * @param port 端口
     * @param callback 回调
     */
    void connect(const std::string& ip, int port, OperationCallback callback);

    /** @brief 是否已连接 */
    bool isConnected() const;

    /** @brief 请求在线玩家列表 */
    void requestOnlinePlayers();

    /** @brief 请求部落成员列表 */
    void requestClanMembers();

    /** @brief 请求部落列表 */
    void requestClanList();

    /** @brief 请求战斗状态 */
    void requestBattleStatus();

    /**
     * @brief 创建部落
     * @param clanName 部落名称
     * @param callback 回调
     */
    void createClan(const std::string& clanName, OperationCallback callback);

    /**
     * @brief 加入部落
     * @param clanId 部落ID
     * @param callback 回调
     */
    void joinClan(const std::string& clanId, OperationCallback callback);

    /**
     * @brief 退出部落
     * @param callback 回调
     */
    void leaveClan(OperationCallback callback);

    /**
     * @brief 发送聊天消息
     * @param message 消息内容
     */
    void sendChatMessage(const std::string& message);

    /**
     * @brief 设置聊天消息回调
     * @param callback 回调函数 (sender, message)
     */
    void setOnChatMessage(std::function<void(const std::string&, const std::string&)> callback);

    /** @brief 初始化（注册网络回调） */
    void initialize();

    /** @brief 清理 */
    void cleanup();

    /** @brief 同步本地账户的部落信息 */
    void syncLocalClanInfo();

private:
    ClanService() = default;

    void registerNetworkCallbacks();
    void parseUserListData(const std::string& data);
    void parseClanMembersData(const std::string& json);
    void parseBattleStatusData(const std::string& json);

    OperationCallback _connectCallback;     ///< 连接回调
    OperationCallback _createClanCallback;  ///< 创建部落回调
    OperationCallback _joinClanCallback;    ///< 加入部落回调
    OperationCallback _leaveClanCallback;   ///< 退出部落回调
    std::function<void(const std::string&, const std::string&)> _chatCallback; ///< 聊天回调
    std::string _pendingClanId;             ///< 待处理部落ID
    std::string _pendingClanName;           ///< 待处理部落名称

    bool _initialized = false;  ///< 是否已初始化
};

#endif // __CLAN_SERVICE_H__