/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Server.h
 * File Function: 服务器主逻辑
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef _SERVER_H_

#define _SERVER_H_

#include <WinSock2.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

// ==================== 数据包类型 ====================

enum PacketType

{

    // 基础功能

    PACKET_LOGIN = 1,

    PACKET_UPLOAD_MAP = 2,

    PACKET_QUERY_MAP = 3,

    PACKET_ATTACK_DATA = 4,

    // 🆕 用户列表
    REQ_USER_LIST = 5,      // 请求可攻击的用户列表
    RESP_USER_LIST = 6,     // 返回用户列表

    // 玩家对战

    PACKET_FIND_MATCH = 10,  // 请求匹配对手

    PACKET_MATCH_FOUND = 11,  // 匹配成功通知

    PACKET_MATCH_CANCEL = 12,  // 取消匹配

    PACKET_ATTACK_START = 13,  // 开始攻击

    PACKET_ATTACK_RESULT = 14,  // 攻击结果

    PACKET_BATTLE_REPLAY = 15,  // 战斗回放

    // 部落系统

    PACKET_CREATE_CLAN = 20,  // 创建部落

    PACKET_JOIN_CLAN = 21,  // 加入部落

    PACKET_LEAVE_CLAN = 22,  // 离开部落

    PACKET_CLAN_LIST = 23,  // 获取部落列表

    PACKET_CLAN_MEMBERS = 24,  // 获取部落成员

    PACKET_CLAN_INFO = 25,  // 部落信息

    // 部落战争

    PACKET_CLAN_WAR_SEARCH = 30,  // 搜索部落战对手

    PACKET_CLAN_WAR_MATCH = 31,  // 部落战匹配成功

    PACKET_CLAN_WAR_ATTACK = 32,  // 部落战攻击

    PACKET_CLAN_WAR_RESULT = 33,  // 部落战结果

    PACKET_CLAN_WAR_STATUS = 34,  // 部落战状态

    // 🆕 实时PVP与观战
    PACKET_PVP_REQUEST = 40,      // 请求挑战/攻击
    PACKET_PVP_START = 41,        // PVP开始通知
    PACKET_PVP_ACTION = 42,       // PVP操作（下兵）
    PACKET_PVP_END = 43,          // PVP结束
    PACKET_SPECTATE_REQUEST = 44, // 请求观战
    PACKET_SPECTATE_JOIN = 45,    // 加入观战通知
    
    // 🆕 部落战争增强
    PACKET_CLAN_WAR_MEMBER_LIST = 50,   // 获取部落战成员列表
    PACKET_CLAN_WAR_ATTACK_START = 51,  // 开始攻击部落战目标
    PACKET_CLAN_WAR_ATTACK_END = 52,    // 部落战攻击结束
    PACKET_CLAN_WAR_SPECTATE = 53,      // 观战部落战
    PACKET_CLAN_WAR_STATE_UPDATE = 54,   // 部落战状态更新

    // 🆕 战斗状态广播
    PACKET_BATTLE_STATUS_LIST = 60,     // 请求/返回战斗状态列表
    PACKET_BATTLE_STATUS_UPDATE = 61    // 战斗状态更新通知
};

// ==================== 数据包头 ====================

struct PacketHeader

{
    uint32_t type;

    uint32_t length;
};

// ==================== 玩家上下文 ====================

struct PlayerContext

{
    SOCKET socket = INVALID_SOCKET;

    std::string playerId;

    std::string playerName;

    std::string clanId;

    std::string mapData;

    int trophies = 0;  // 奖杯数

    int gold = 1000;  // 金币

    int elixir = 1000;  // 圣水

    bool isSearchingMatch = false;

    std::chrono::steady_clock::time_point matchStartTime;
};

// ==================== 攻击结果 ====================

struct AttackResult

{
    std::string attackerId;

    std::string defenderId;

    int starsEarned = 0;  // 0-3 星

    int goldLooted = 0;

    int elixirLooted = 0;

    int trophyChange = 0;

    std::string replayData;
};

// ==================== 部落信息 ====================

struct ClanInfo

{
    std::string clanId;

    std::string clanName;

    std::string leaderId;

    std::string description;

    std::vector<std::string> memberIds;

    int clanTrophies = 0;

    int requiredTrophies = 0;  // 加入所需奖杯

    bool isOpen = true;  // 是否开放加入
};

// ==================== 部落战争 ====================

struct ClanWarInfo

{
    std::string warId;

    std::string clan1Id;

    std::string clan2Id;

    int clan1Stars = 0;

    int clan2Stars = 0;

    std::vector<AttackResult> attacks;

    std::chrono::steady_clock::time_point startTime;

    std::chrono::steady_clock::time_point endTime;

    bool isActive = false;
};

// ==================== 攻击记录 ====================
struct AttackRecord
{
    std::string attackerId;
    std::string attackerName;
    int starsEarned = 0;
    float destructionRate = 0.0f;  // 0.0 ~ 1.0
    std::chrono::steady_clock::time_point attackTime;
};

// 🆕 PVP会话 (需要在ClanWarSession之前定义)
struct PvpSession
{
    std::string attackerId;
    std::string defenderId;
    std::vector<std::string> spectatorIds;
    std::string mapData; // 缓存地图数据
    bool isActive = true;
};

// ==================== 部落战成员信息 ====================
struct ClanWarMember
{
    std::string memberId;
    std::string memberName;
    std::string mapData;           // 成员的基地数据
    int bestStars = 0;             // 被攻击的最高星数
    float bestDestructionRate = 0.0f; // 被攻击的最高摧毁率
    std::vector<AttackRecord> attacksReceived; // 受到的攻击记录
};

// ==================== 部落战会话 ====================
struct ClanWarSession
{
    std::string warId;
    std::string clan1Id;
    std::string clan2Id;
    
    // 双方成员信息
    std::vector<ClanWarMember> clan1Members;
    std::vector<ClanWarMember> clan2Members;
    
    // 当前进行中的战斗
    std::map<std::string, PvpSession> activeBattles; // key: attackerId
    
    // 战争状态
    int clan1TotalStars = 0;
    int clan2TotalStars = 0;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    bool isActive = true;
};

// ==================== 匹配队列项 ====================

struct MatchQueueEntry

{
    SOCKET socket;

    std::string playerId;

    int trophies;

    std::chrono::steady_clock::time_point queueTime;
};

// ==================== 服务器类 ====================

class Server

{
public:
    Server();

    ~Server();

    void run();

    friend void clientHandler(SOCKET clientSocket, Server& server);

private:
    WSADATA wsaData;

    SOCKET serverSocket;

    struct sockaddr_in serverAddr;

    int port;

    // 玩家数据

    std::map<SOCKET, PlayerContext> onlinePlayers;

    std::map<std::string, std::string> savedMaps;  // playerId -> mapData

    std::map<std::string, PlayerContext> playerDatabase;  // playerId -> PlayerContext (持久化)

    // 匹配队列

    std::vector<MatchQueueEntry> matchQueue;

    std::mutex matchQueueMutex;

    // 部落数据

    std::map<std::string, ClanInfo> clans;  // clanId -> ClanInfo

    std::mutex clanMutex;

    // 部落战争

    std::map<std::string, ClanWarInfo> activeWars;  // warId -> ClanWarInfo

    std::vector<std::string> clanWarQueue;  // 正在搜索部落战的部落

    std::mutex warMutex;

    // 🆕 PVP会话管理
    std::map<std::string, PvpSession> pvpSessions; // key: attackerId (or unique session id)
    std::mutex pvpMutex;
    
    // 🆕 部落战会话管理
    std::map<std::string, ClanWarSession> clanWarSessions; // warId -> ClanWarSession
    std::mutex clanWarSessionMutex;

    std::mutex dataMutex;

    // 网络函数

    void createAndBindSocket();

    void handleConnections();

    void closeClientSocket(SOCKET clientSocket);

    static bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);

    static bool recvPacket(SOCKET socket, uint32_t& outType, std::string& outData);

    static bool recvFixedAmount(SOCKET socket, char* buffer, int totalBytes);

    // 匹配系统

    void addToMatchQueue(SOCKET socket);

    void removeFromMatchQueue(SOCKET socket);

    void processMatchQueue();

    PlayerContext* findMatchForPlayer(const PlayerContext& player);

    // 部落系统

    std::string generateClanId();

    bool createClan(const std::string& playerId, const std::string& clanName);

    bool joinClan(const std::string& playerId, const std::string& clanId);

    bool leaveClan(const std::string& playerId);

    std::string getClanListJson();

    std::string getClanMembersJson(const std::string& clanId);

    // 部落战争

    std::string generateWarId();

    void addToClanWarQueue(const std::string& clanId);

    void processClanWarQueue();

    void startClanWar(const std::string& clan1Id, const std::string& clan2Id);

    void processClanWarAttack(const std::string& warId, const AttackResult& result);

    // 🆕 PVP系统
    void handlePvpRequest(SOCKET clientSocket, const std::string& targetId);
    void handlePvpAction(SOCKET clientSocket, const std::string& actionData);
    void handleSpectateRequest(SOCKET clientSocket, const std::string& targetId);
    void endPvpSession(const std::string& attackerId);
    
    // 🆕 部落战争增强
    ClanWarSession* getClanWarSession(const std::string& warId);
    void initClanWarMembers(ClanWarSession& session);
    std::string getClanWarMemberListJson(const std::string& warId, const std::string& requesterId);
    void handleClanWarAttackStart(SOCKET clientSocket, const std::string& warId, const std::string& targetId);
    void handleClanWarAttackEnd(const std::string& warId, const AttackRecord& record);
    void handleClanWarSpectate(SOCKET clientSocket, const std::string& warId, const std::string& targetId);
    void broadcastClanWarStateUpdate(const std::string& warId);

    // 辅助函数

    std::string serializeAttackResult(const AttackResult& result);

    AttackResult deserializeAttackResult(const std::string& data);

    SOCKET findSocketByPlayerId(const std::string& playerId);
    
    // 🆕 获取用户列表
    std::string getUserListJson(const std::string& requesterId);

    // 🆕 战斗状态广播
    std::string getBattleStatusListJson();
    void broadcastBattleStatusToAll();
};

#endif