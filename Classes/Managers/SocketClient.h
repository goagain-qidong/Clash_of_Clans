/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SocketClient.h
 * File Function: 客户端网络通信管理器
 * Author:        赵崇治
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef SOCKET_CLIENT_H_
#define SOCKET_CLIENT_H_

// ============================================================================
// 平台相关头文件
// ============================================================================
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "cocos2d.h"

// ============================================================================
// 数据包类型枚举（与服务器 Protocol.h 保持一致）
// ============================================================================
enum PacketType : uint32_t {
    // 基础功能 (1-9)
    PACKET_LOGIN = 1,
    PACKET_UPLOAD_MAP = 2,
    PACKET_QUERY_MAP = 3,
    PACKET_ATTACK_DATA = 4,
    PACKET_USER_LIST_REQ = 5,
    PACKET_USER_LIST_RESP = 6,

    // 匹配系统 (10-19)
    PACKET_MATCH_FIND = 10,
    PACKET_MATCH_FOUND = 11,
    PACKET_MATCH_CANCEL = 12,
    PACKET_ATTACK_START = 13,
    PACKET_ATTACK_RESULT = 14,
    PACKET_BATTLE_REPLAY = 15,

    // 部落系统 (20-29)
    PACKET_CLAN_CREATE = 20,
    PACKET_CLAN_JOIN = 21,
    PACKET_CLAN_LEAVE = 22,
    PACKET_CLAN_LIST = 23,
    PACKET_CLAN_MEMBERS = 24,
    PACKET_CLAN_INFO = 25,
    PACKET_CLAN_CHAT = 26,
    PACKET_CHAT_MESSAGE = 27,

    // 部落战争基础 (30-39)
    PACKET_WAR_SEARCH = 30,
    PACKET_WAR_MATCH = 31,
    PACKET_WAR_ATTACK = 32,
    PACKET_WAR_RESULT = 33,
    PACKET_WAR_STATUS = 34,
    PACKET_WAR_END = 35,

    // PVP 实时对战 (40-49)
    PACKET_PVP_REQUEST = 40,
    PACKET_PVP_START = 41,
    PACKET_PVP_ACTION = 42,
    PACKET_PVP_END = 43,
    PACKET_SPECTATE_REQUEST = 44,
    PACKET_SPECTATE_JOIN = 45,

    // 部落战争增强 (50-59)
    PACKET_WAR_MEMBER_LIST = 50,
    PACKET_WAR_ATTACK_START = 51,
    PACKET_WAR_ATTACK_END = 52,
    PACKET_WAR_SPECTATE = 53,
    PACKET_WAR_STATE_UPDATE = 54,

    // 战斗状态广播 (60-69)
    PACKET_BATTLE_STATUS_LIST = 60,
    PACKET_BATTLE_STATUS_UPDATE = 61
};

// ============================================================================
// 数据包头结构
// ============================================================================
struct PacketHeader {
    uint32_t type;    // 数据包类型
    uint32_t length;  // 数据长度
};

// ============================================================================
// 数据模型结构体
// ============================================================================

/**
 * @struct AttackResult
 * @brief 攻击结果数据
 */
struct AttackResult {
    std::string attacker_id;   // 攻击者 ID
    std::string defender_id;   // 防守者 ID
    int stars_earned = 0;      // 获得星数
    int gold_looted = 0;       // 掠夺金币
    int elixir_looted = 0;     // 掠夺圣水
    int trophy_change = 0;     // 奖杯变化
    std::string replay_data;   // 回放数据

    std::string Serialize() const;
    static AttackResult Deserialize(const std::string& data);
};

/**
 * @struct MatchInfo
 * @brief 匹配信息
 */
struct MatchInfo {
    std::string opponent_id;    // 对手 ID
    int opponent_trophies = 0;  // 对手奖杯
};

/**
 * @struct ClanInfoClient
 * @brief 部落信息（客户端视图）
 */
struct ClanInfoClient {
    std::string clan_id;         // 部落 ID
    std::string clan_name;       // 部落名称
    int member_count = 0;        // 成员数量
    int clan_trophies = 0;       // 部落奖杯
    int required_trophies = 0;   // 加入所需奖杯
    bool is_open = true;         // 是否开放加入
};

/**
 * @struct SpectateInfo
 * @brief 观战信息
 */
struct SpectateInfo {
    bool success = false;                   // 是否成功加入
    std::string attacker_id;                // 攻击者 ID
    std::string defender_id;                // 防守者 ID
    std::string map_data;                   // 地图数据
    int64_t elapsed_ms = 0;                 // 已进行时间（毫秒）
    std::vector<std::string> action_history; // 操作历史
};

// ============================================================================
// 回调类型定义
// ============================================================================
namespace SocketCallback {
    // 连接相关
    using OnConnected = std::function<void(bool success)>;
    using OnDisconnected = std::function<void()>;
    
    // 登录相关
    using OnLoginResult = std::function<void(bool success, const std::string& message)>;
    
    // 匹配相关
    using OnMatchFound = std::function<void(const MatchInfo& info)>;
    using OnMatchCancelled = std::function<void()>;
    
    // 攻击相关
    using OnAttackStart = std::function<void(const std::string& map_data)>;
    using OnAttackResult = std::function<void(const AttackResult& result)>;
    
    // PVP 相关
    using OnPvpStart = std::function<void(const std::string& role, 
                                          const std::string& opponent_id, 
                                          const std::string& map_data)>;
    using OnPvpAction = std::function<void(int unit_type, float x, float y)>;
    using OnPvpEnd = std::function<void(const std::string& reason)>;
    
    // 观战相关
    using OnSpectateJoin = std::function<void(const SpectateInfo& info)>;
    
    // 用户列表
    using OnUserListReceived = std::function<void(const std::string& data)>;
    using OnMapReceived = std::function<void(const std::string& data)>;
    using OnBattleStatusList = std::function<void(const std::string& data)>;
    
    // 部落相关
    using OnClanCreated = std::function<void(bool success, const std::string& clan_id)>;
    using OnClanJoined = std::function<void(bool success)>;
    using OnClanLeft = std::function<void(bool success)>;
    using OnClanList = std::function<void(const std::vector<ClanInfoClient>& clans)>;
    using OnClanMembers = std::function<void(const std::string& data)>;
    using OnChatMessage = std::function<void(const std::string& sender, const std::string& message)>;
    
    // 部落战争相关
    using OnClanWarMatch = std::function<void(const std::string& war_id, 
                                              const std::string& clan1_id, 
                                              const std::string& clan2_id)>;
    using OnClanWarStatus = std::function<void(const std::string& war_id, 
                                               int stars1, int stars2)>;
    using OnClanWarMemberList = std::function<void(const std::string& data)>;
    using OnClanWarAttackStart = std::function<void(const std::string& type, 
                                                     const std::string& target_id, 
                                                     const std::string& map_data)>;
    using OnClanWarSpectate = std::function<void(bool success, 
                                                  const std::string& attacker_id, 
                                                  const std::string& defender_id, 
                                                  const std::string& map_data)>;
    using OnClanWarStateUpdate = std::function<void(const std::string& data)>;
}

// ============================================================================
// SocketClient 类
// ============================================================================

/**
 * @class SocketClient
 * @brief 客户端网络通信管理器（单例模式）
 * 
 * 职责：
 * - 管理与服务器的 TCP 连接
 * - 收发网络数据包
 * - 在主线程中触发回调
 * 
 * 线程模型：
 * - 接收线程：后台运行，接收服务器数据
 * - 主线程：通过 processCallbacks() 处理回调
 */
class SocketClient {
 public:
    /**
     * @brief 获取单例实例
     */
    static SocketClient& getInstance();

    // ======================== 连接管理 ========================
    
    /**
     * @brief 连接到服务器
     * @param host 服务器地址
     * @param port 服务器端口
     * @return 是否连接成功
     */
    bool connect(const std::string& host, int port);
    
    /**
     * @brief 断开与服务器的连接
     */
    void disconnect();
    
    /**
     * @brief 检查是否已连接
     */
    bool isConnected() const;

    // ======================== 基础功能 ========================
    
    void login(const std::string& player_id, 
               const std::string& player_name, 
               int trophies,
               const std::string& clan_id = "");
    void uploadMap(const std::string& map_data);
    void queryMap(const std::string& target_id);
    void requestUserList();

    // ======================== 玩家对战 ========================
    
    void findMatch();
    void cancelMatch();
    void startAttack(const std::string& target_id);
    void submitAttackResult(const AttackResult& result);

    // ======================== 部落系统 ========================
    
    void createClan(const std::string& clan_name);
    void joinClan(const std::string& clan_id);
    void leaveClan();
    void getClanList();
    void getClanMembers(const std::string& clan_id);
    void sendChatMessage(const std::string& message);

    // ======================== 部落战争 ========================
    
    void searchClanWar();
    void attackInClanWar(const std::string& war_id, 
                         const std::string& target_member_id);
    void submitClanWarResult(const std::string& war_id, 
                             const AttackResult& result);
    void requestClanWarMemberList(const std::string& war_id);
    void startClanWarAttack(const std::string& war_id, 
                            const std::string& target_id);
    void endClanWarAttack(const std::string& war_id, 
                          int stars, 
                          float destruction_rate);
    void spectateClanWar(const std::string& war_id, 
                         const std::string& target_id);

    // ======================== PVP 系统 ========================
    
    /**
     * @brief 发起 PVP 请求
     * @param target_id 目标玩家 ID
     */
    void requestPvp(const std::string& target_id);
    
    /**
     * @brief 发送 PVP 操作（单位部署）
     * @param unit_type 单位类型
     * @param x 部署 X 坐标
     * @param y 部署 Y 坐标
     */
    void sendPvpAction(int unit_type, float x, float y);
    
    /**
     * @brief 结束 PVP 战斗
     */
    void endPvp();
    
    /**
     * @brief 请求观战
     * @param target_id 目标玩家 ID（攻击者或防守者均可）
     */
    void requestSpectate(const std::string& target_id);
    
    /**
     * @brief 请求战斗状态列表
     */
    void requestBattleStatusList();

    // ======================== 回调设置 ========================
    
    void setOnConnected(SocketCallback::OnConnected callback);
    void setOnDisconnected(SocketCallback::OnDisconnected callback);
    void setOnLoginResult(SocketCallback::OnLoginResult callback);
    void setOnMatchFound(SocketCallback::OnMatchFound callback);
    void setOnMatchCancelled(SocketCallback::OnMatchCancelled callback);
    void setOnAttackStart(SocketCallback::OnAttackStart callback);
    void setOnAttackResult(SocketCallback::OnAttackResult callback);
    void setOnUserListReceived(SocketCallback::OnUserListReceived callback);
    void setOnMapReceived(SocketCallback::OnMapReceived callback);
    void setOnBattleStatusList(SocketCallback::OnBattleStatusList callback);
    
    // 部落回调
    void setOnClanCreated(SocketCallback::OnClanCreated callback);
    void setOnClanJoined(SocketCallback::OnClanJoined callback);
    void setOnClanLeft(SocketCallback::OnClanLeft callback);
    void setOnClanList(SocketCallback::OnClanList callback);
    void setOnClanMembers(SocketCallback::OnClanMembers callback);
    void setOnChatMessage(SocketCallback::OnChatMessage callback);
    
    // 部落战争回调
    void setOnClanWarMatch(SocketCallback::OnClanWarMatch callback);
    void setOnClanWarStatus(SocketCallback::OnClanWarStatus callback);
    void setOnClanWarMemberList(SocketCallback::OnClanWarMemberList callback);
    void setOnClanWarAttackStart(SocketCallback::OnClanWarAttackStart callback);
    void setOnClanWarSpectate(SocketCallback::OnClanWarSpectate callback);
    void setOnClanWarStateUpdate(SocketCallback::OnClanWarStateUpdate callback);
    
    // PVP 回调
    void setOnPvpStart(SocketCallback::OnPvpStart callback);
    void setOnPvpAction(SocketCallback::OnPvpAction callback);
    void setOnPvpEnd(SocketCallback::OnPvpEnd callback);
    void setOnSpectateJoin(SocketCallback::OnSpectateJoin callback);

    // ======================== 主线程回调处理 ========================
    
    /**
     * @brief 在主线程中处理待处理的回调
     * @note 应在每帧 update 中调用
     */
    void processCallbacks();

 private:
    // 禁用拷贝和移动
    SocketClient();
    ~SocketClient();
    SocketClient(const SocketClient&) = delete;
    SocketClient& operator=(const SocketClient&) = delete;

    // ======================== 内部数据结构 ========================
    
    struct ReceivedPacket {
        uint32_t type;
        std::string data;
    };

    // ======================== 网络操作 ========================
    
    bool sendPacket(uint32_t type, const std::string& data);
    bool recvPacket(uint32_t& out_type, std::string& out_data);
    bool recvFixedAmount(char* buffer, int total_bytes);
    void recvThreadFunc();
    void handlePacket(uint32_t type, const std::string& data);
    
    // ======================== 消息解析辅助 ========================
    
    void handlePvpStart(const std::string& data);
    void handlePvpAction(const std::string& data);
    void handleSpectateJoin(const std::string& data);
    void handleClanList(const std::string& data);

    // ======================== 成员变量 ========================
    
    SOCKET socket_ = INVALID_SOCKET;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    std::thread recv_thread_;
    
    std::mutex send_mutex_;           // 保护发送操作
    std::mutex callback_mutex_;       // 保护回调队列
    std::queue<ReceivedPacket> pending_packets_;

    // 回调函数存储
    SocketCallback::OnConnected on_connected_;
    SocketCallback::OnDisconnected on_disconnected_;
    SocketCallback::OnLoginResult on_login_result_;
    SocketCallback::OnMatchFound on_match_found_;
    SocketCallback::OnMatchCancelled on_match_cancelled_;
    SocketCallback::OnAttackStart on_attack_start_;
    SocketCallback::OnAttackResult on_attack_result_;
    SocketCallback::OnUserListReceived on_user_list_received_;
    SocketCallback::OnMapReceived on_map_received_;
    SocketCallback::OnBattleStatusList on_battle_status_list_;
    
    SocketCallback::OnClanCreated on_clan_created_;
    SocketCallback::OnClanJoined on_clan_joined_;
    SocketCallback::OnClanLeft on_clan_left_;
    SocketCallback::OnClanList on_clan_list_;
    SocketCallback::OnClanMembers on_clan_members_;
    SocketCallback::OnChatMessage on_chat_message_;
    
    SocketCallback::OnClanWarMatch on_clan_war_match_;
    SocketCallback::OnClanWarStatus on_clan_war_status_;
    SocketCallback::OnClanWarMemberList on_clan_war_member_list_;
    SocketCallback::OnClanWarAttackStart on_clan_war_attack_start_;
    SocketCallback::OnClanWarSpectate on_clan_war_spectate_;
    SocketCallback::OnClanWarStateUpdate on_clan_war_state_update_;
    
    SocketCallback::OnPvpStart on_pvp_start_;
    SocketCallback::OnPvpAction on_pvp_action_;
    SocketCallback::OnPvpEnd on_pvp_end_;
    SocketCallback::OnSpectateJoin on_spectate_join_;

#ifdef _WIN32
    bool wsa_initialized_ = false;
#endif
};

#endif  // SOCKET_CLIENT_H_