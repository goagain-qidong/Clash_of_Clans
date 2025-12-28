/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanHall.h
 * File Function: 部落系统管理
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "ClanInfo.h"
#include "PlayerRegistry.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

/**
 * @class ClanHall
 * @brief 管理部落的创建、加入、离开及信息查询。
 *
 * ClanHall 是部落系统的核心管理类，负责处理所有与部落相关的操作。
 * 它维护一个部落映射表，并提供线程安全的访问接口。
 *
 * 主要功能：
 * - 创建新部落（创建者自动成为族长）
 * - 玩家加入/离开部落
 * - 查询部落列表和成员信息
 * - 验证玩家的部落归属关系
 *
 * 部落生命周期：
 * 1. 玩家创建部落时，该玩家成为族长
 * 2. 其他玩家可以加入开放的部落
 * 3. 当所有成员离开时，部落自动解散
 *
 * 线程安全：
 * 所有公共方法都是线程安全的，内部使用互斥锁保护部落数据。
 *
 * @note 此类依赖 PlayerRegistry 来获取和更新玩家信息。
 *
 * @see ClanInfo
 * @see PlayerRegistry
 */
class ClanHall {
 public:
    /**
     * @brief 构造函数。
     *
     * @param registry 玩家注册表指针，用于获取和更新玩家的部落信息。
     *                 调用者需保证 registry 在 ClanHall 生命周期内有效。
     */
    explicit ClanHall(PlayerRegistry* registry);

    /**
     * @brief 创建新部落。
     *
     * 创建一个新部落，创建者自动成为族长和第一个成员。
     * 创建成功后，玩家的 clanId 会被更新为新部落的ID。
     *
     * 失败条件：
     * - 玩家不存在（未登录）
     * - 玩家已经属于某个部落
     *
     * @param player_id 创建者的玩家ID
     * @param clan_name 部落名称
     * @return 创建成功返回 true，失败返回 false
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    bool CreateClan(const std::string& player_id, const std::string& clan_name);

    /**
     * @brief 加入指定部落。
     *
     * 将玩家添加到指定部落的成员列表中，并更新玩家的 clanId。
     *
     * 失败条件：
     * - 玩家不存在
     * - 玩家已经属于某个部落
     * - 目标部落不存在
     * - 目标部落未开放加入
     * - 玩家奖杯数不满足部落要求
     *
     * @param player_id 要加入的玩家ID
     * @param clan_id 目标部落ID
     * @return 加入成功返回 true，失败返回 false
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    bool JoinClan(const std::string& player_id, const std::string& clan_id);

    /**
     * @brief 离开当前部落。
     *
     * 将玩家从其所属部落中移除，并清空玩家的 clanId。
     * 如果离开后部落没有成员，部落将被自动删除。
     *
     * 失败条件：
     * - 玩家不存在
     * - 玩家不属于任何部落
     *
     * @param player_id 要离开的玩家ID
     * @return 离开成功返回 true，失败返回 false
     *
     * @note 目前族长离开部落时，部落不会自动转让，可能导致部落无族长。
     * @note 线程安全：此方法内部加锁保护。
     */
    bool LeaveClan(const std::string& player_id);

    /**
     * @brief 获取所有部落的 JSON 列表。
     *
     * 返回格式示例：
     * @code
     * [
     *   {
     *     "id": "CLAN_1",
     *     "name": "部落名称",
     *     "members": 5,
     *     "trophies": 12500,
     *     "required": 1000,
     *     "open": true
     *   },
     *   ...
     * ]
     * @endcode
     *
     * @return JSON 格式的部落列表字符串
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    std::string GetClanListJson();

    /**
     * @brief 获取指定部落的成员 JSON 列表。
     *
     * 返回格式示例：
     * @code
     * {
     *   "members": [
     *     {"id": "player1", "name": "玩家1", "trophies": 2500, "online": true},
     *     ...
     *   ]
     * }
     * @endcode
     *
     * @param clan_id 部落ID
     * @return JSON 格式的成员列表字符串，部落不存在时返回错误JSON
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    std::string GetClanMembersJson(const std::string& clan_id);

    /**
     * @brief 检查玩家是否在指定部落中。
     *
     * @param player_id 玩家ID
     * @param clan_id 部落ID
     * @return 玩家在该部落中返回 true，否则返回 false
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    bool IsPlayerInClan(const std::string& player_id,
                        const std::string& clan_id);

    /**
     * @brief 获取部落的所有成员ID列表。
     *
     * @param clan_id 部落ID
     * @return 成员ID列表，部落不存在时返回空列表
     *
     * @note 返回的是副本，调用者可以安全使用。
     * @note 线程安全：此方法内部加锁保护。
     */
    std::vector<std::string> GetClanMemberIds(const std::string& clan_id);

    /**
     * @brief 确保玩家在指定部落中（用于登录时恢复部落归属）。
     *
     * 如果部落存在且玩家不在成员列表中，则添加玩家到部落。
     * 如果部落不存在，则直接返回，不创建临时部落。
     *
     * @param player_id 玩家ID
     * @param clan_id 部落ID
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    void EnsurePlayerInClan(const std::string& player_id, const std::string& clan_id);

 private:
    std::map<std::string, ClanInfo> clans_;  ///< 部落映射表（部落ID -> 部落信息）
    std::mutex clan_mutex_;                   ///< 保护 clans_ 的互斥锁
    PlayerRegistry* player_registry_;         ///< 玩家注册表指针（非拥有）
    std::string data_file_path_;              ///< 部落数据文件路径
    int clan_id_counter_;                     ///< 部落ID计数器

    /**
     * @brief 生成唯一的部落ID。
     *
     * 使用计数器生成格式为 "CLAN_xxx" 的唯一标识符。
     *
     * @return 新生成的部落ID字符串
     *
     * @note 此方法不是线程安全的，应在持有 clan_mutex_ 时调用。
     */
    std::string GenerateClanId();

    /**
     * @brief 从文件加载部落数据。
     *
     * 在构造时调用，从本地文件恢复所有部落信息。
     * 如果文件不存在或格式错误，则跳过加载。
     */
    void LoadFromFile();

    /**
     * @brief 将部落数据保存到文件。
     *
     * 在部落创建、加入、离开等操作后调用，持久化当前状态。
     *
     * @note 应在持有 clan_mutex_ 时调用。
     */
    void SaveToFile();
};