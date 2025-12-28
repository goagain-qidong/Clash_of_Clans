/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Protocol.h
 * File Function: 网络协议定义（客户端与服务器共享）
 * Author:        赵崇治
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <cstdint>
#include <string>

// ============================================================================
// 数据包类型定义
// ============================================================================
//
// 命名规范：PACKET_<模块>_<动作>
// 数值分配：每个模块占用 10 个数值空间，便于扩展
//
// 通信模式：
// - 请求/响应模式：客户端发送请求，服务器返回响应（使用相同的包类型）
// - 推送模式：服务器主动向客户端推送通知
//
// ============================================================================

enum PacketType : uint32_t {
    // ======================== 基础功能 (1-9) ========================
    // 玩家登录、地图上传/查询、用户列表等基础操作
    PACKET_LOGIN = 1,           ///< 登录请求/响应
    PACKET_UPLOAD_MAP = 2,      ///< 上传地图数据（无响应）
    PACKET_QUERY_MAP = 3,       ///< 查询地图数据
    PACKET_ATTACK_DATA = 4,     ///< 攻击数据（已废弃）
    PACKET_USER_LIST_REQ = 5,   ///< 请求在线用户列表
    PACKET_USER_LIST_RESP = 6,  ///< 响应在线用户列表

    // ======================== 匹配系统 (10-19) ========================
    // 普通匹配战斗相关
    PACKET_MATCH_FIND = 10,     ///< 发起匹配请求
    PACKET_MATCH_FOUND = 11,    ///< 匹配成功通知（服务器推送）
    PACKET_MATCH_CANCEL = 12,   ///< 取消匹配
    PACKET_ATTACK_START = 13,   ///< 开始攻击（请求目标地图）
    PACKET_ATTACK_RESULT = 14,  ///< 攻击结果上报
    PACKET_BATTLE_REPLAY = 15,  ///< 战斗回放数据

    // ======================== 部落系统 (20-29) ========================
    // 部落创建、加入、离开、信息查询等
    PACKET_CLAN_CREATE = 20,    ///< 创建部落
    PACKET_CLAN_JOIN = 21,      ///< 加入部落
    PACKET_CLAN_LEAVE = 22,     ///< 离开部落
    PACKET_CLAN_LIST = 23,      ///< 获取部落列表
    PACKET_CLAN_MEMBERS = 24,   ///< 获取部落成员
    PACKET_CLAN_INFO = 25,      ///< 获取部落信息
    PACKET_CLAN_CHAT = 26,      ///< 发送部落聊天消息
    PACKET_CHAT_MESSAGE = 27,   ///< 接收部落聊天消息

    // ======================== 部落战争基础 (30-39) ========================
    // 部落战争的搜索、匹配、攻击等
    PACKET_WAR_SEARCH = 30,       ///< 搜索部落战（发起匹配）
    PACKET_WAR_MATCH = 31,        ///< 部落战匹配成功（服务器推送）
    PACKET_WAR_ATTACK = 32,       ///< 部落战攻击（请求目标地图）
    PACKET_WAR_RESULT = 33,       ///< 部落战攻击结果上报
    PACKET_WAR_STATUS = 34,       ///< 部落战状态更新
    PACKET_WAR_END = 35,          ///< 部落战结束通知

    // ======================== PVP 实时对战 (40-49) ========================
    // 玩家之间的实时 PVP 战斗和观战
    PACKET_PVP_REQUEST = 40,      ///< 发起 PVP 请求
    PACKET_PVP_START = 41,        ///< PVP 开始通知（双方）
    PACKET_PVP_ACTION = 42,       ///< PVP 操作同步（单位部署）
    PACKET_PVP_END = 43,          ///< PVP 结束通知
    PACKET_SPECTATE_REQUEST = 44, ///< 观战请求
    PACKET_SPECTATE_JOIN = 45,    ///< 观战加入响应（含历史操作）

    // ======================== 部落战争增强 (50-59) ========================
    // 部落战争中的实时战斗和观战功能
    PACKET_WAR_MEMBER_LIST = 50,    ///< 部落战成员列表
    PACKET_WAR_ATTACK_START = 51,   ///< 部落战攻击开始
    PACKET_WAR_ATTACK_END = 52,     ///< 部落战攻击结束
    PACKET_WAR_SPECTATE = 53,       ///< 部落战观战
    PACKET_WAR_STATE_UPDATE = 54,   ///< 部落战状态更新（服务器推送）

    // ======================== 战斗状态广播 (60-69) ========================
    // 全局战斗状态，用于更新用户列表中的战斗标记
    PACKET_BATTLE_STATUS_LIST = 60,   ///< 战斗状态列表（所有活跃战斗）
    PACKET_BATTLE_STATUS_UPDATE = 61  ///< 战斗状态更新（服务器推送）
};

// ============================================================================
// 数据包头结构
// ============================================================================
//
// 网络数据包格式：[PacketHeader][Payload]
// - PacketHeader：固定 8 字节，包含包类型和载荷长度
// - Payload：可变长度，具体格式由包类型决定
//
// ============================================================================

/**
 * @struct PacketHeader
 * @brief 网络数据包头，描述包类型和载荷长度。
 *
 * 所有网络通信都使用此结构作为数据包头。发送时先发送头部，
 * 再发送对应长度的载荷数据。
 *
 * @note 字节序：使用主机字节序，客户端和服务器应在同一平台或处理字节序转换。
 */
struct PacketHeader {
    uint32_t type;    ///< 数据包类型（PacketType 枚举值）
    uint32_t length;  ///< 载荷长度（不含包头的 8 字节）
};

// ============================================================================
// 协议数据格式常量
// ============================================================================
//
// 定义协议中使用的分隔符和标记，用于解析和构建消息内容。
//
// ============================================================================

/**
 * @namespace ProtocolFormat
 * @brief 协议格式常量，定义消息解析所需的分隔符和标记。
 */
namespace ProtocolFormat {
    /// 字段分隔符，用于分隔消息中的各个字段
    /// 示例："field1|field2|field3"
    constexpr char kFieldSeparator = '|';
    
    /// 历史记录标记，用于分隔消息主体和历史操作记录
    /// 示例："data[[[HISTORY]]]action1[[[ACTION]]]action2"
    constexpr const char* kHistoryMarker = "[[[HISTORY]]]";
    
    /// 操作分隔符，用于分隔历史记录中的多个操作
    constexpr const char* kActionSeparator = "[[[ACTION]]]";
    
    /// PVP 操作数据字段分隔符（更简洁的逗号分隔）
    /// 示例："elapsedMs,unitType,x,y"
    constexpr char kActionFieldSeparator = ',';
}

// ============================================================================
// PVP 响应类型常量
// ============================================================================
//
// PVP_START 响应的第一个字段，表示请求结果。
//
// ============================================================================

/**
 * @namespace PvpResponse
 * @brief PVP 响应常量，定义 PVP 请求结果的类型和原因。
 */
namespace PvpResponse {
    // 角色类型（成功响应的第一个字段）
    constexpr const char* kRoleAttack = "ATTACK";   ///< 作为攻击者
    constexpr const char* kRoleDefend = "DEFEND";   ///< 作为防守者
    constexpr const char* kRoleFail = "FAIL";       ///< 请求失败
    
    // 失败原因（失败响应的第二个字段）
    constexpr const char* kReasonNotLoggedIn = "NOT_LOGGED_IN";       ///< 未登录
    constexpr const char* kReasonTargetOffline = "TARGET_OFFLINE";    ///< 目标离线
    constexpr const char* kReasonNoMap = "NO_MAP";                    ///< 目标无地图
    constexpr const char* kReasonAlreadyInBattle = "ALREADY_IN_BATTLE"; ///< 已在战斗中
    constexpr const char* kReasonTargetInBattle = "TARGET_IN_BATTLE";   ///< 目标在战斗中
    constexpr const char* kReasonCannotAttackSelf = "CANNOT_ATTACK_SELF"; ///< 不能攻击自己
}

// ============================================================================
// 战斗结束原因
// ============================================================================
//
// PVP_END 消息的第一个字段，表示战斗结束的原因。
//
// ============================================================================

/**
 * @namespace BattleEndReason
 * @brief 战斗结束原因常量。
 */
namespace BattleEndReason {
    constexpr const char* kBattleEnded = "BATTLE_ENDED";                ///< 正常结束
    constexpr const char* kOpponentDisconnected = "OPPONENT_DISCONNECTED"; ///< 对手断开
    constexpr const char* kDefenderDisconnected = "DEFENDER_DISCONNECTED"; ///< 防守方断开
    constexpr const char* kWarEnded = "WAR_ENDED";                      ///< 部落战争结束
}