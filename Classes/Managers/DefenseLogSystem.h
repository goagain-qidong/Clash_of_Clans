#pragma once
#ifndef __DEFENSE_LOG_SYSTEM_H__
#define __DEFENSE_LOG_SYSTEM_H__

#include "cocos2d.h"
#include "SocketClient.h"
#include <string>
#include <vector>

/**
 * @struct DefenseLog
 * @brief 防守日志记录
 */
struct DefenseLog
{
    std::string attackerId;      // 攻击者ID
    std::string attackerName;    // 攻击者名称
    int starsLost = 0;          // 失去星数
    int goldLost = 0;           // 失去金币
    int elixirLost = 0;         // 失去圣水
    int trophyChange = 0;       // 奖杯变化
    std::string timestamp;      // 时间戳
    bool isViewed = false;      // 是否已查看
    std::string replayData;     // 🆕 回放数据
    
    std::string serialize() const;
    static DefenseLog deserialize(const std::string& data);
};

/**
 * @class DefenseLogSystem
 * @brief 防守日志系统 - 管理被攻击记录
 */
class DefenseLogSystem
{
public:
    static DefenseLogSystem& getInstance();
    
    /**
     * @brief 添加防守日志
     */
    void addDefenseLog(const DefenseLog& log);
    
    /**
     * @brief 获取所有未查看的日志
     */
    std::vector<DefenseLog> getUnviewedLogs() const;
    
    /**
     * @brief 获取所有日志
     */
    const std::vector<DefenseLog>& getAllLogs() const { return _logs; }
    
    /**
     * @brief 标记所有日志为已查看
     */
    void markAllAsViewed();
    
    /**
     * @brief 清空所有日志
     */
    void clearAllLogs();
    
    /**
     * @brief 保存日志到本地
     */
    void save();
    
    /**
     * @brief 从本地加载日志
     */
    void load();
    
    /**
     * @brief 检查是否有未查看的日志
     */
    bool hasUnviewedLogs() const;
    
    /**
     * @brief 显示防守日志UI
     */
    void showDefenseLogUI();
    
    /**
     * @brief 🆕 显示攻击详情弹窗
     */
    static void showAttackDetailPopup(const cocos2d::Size& visibleSize, cocos2d::Scene* scene, const DefenseLog& log);
    
private:
    DefenseLogSystem() = default;
    ~DefenseLogSystem() = default;
    DefenseLogSystem(const DefenseLogSystem&) = delete;
    DefenseLogSystem& operator=(const DefenseLogSystem&) = delete;
    
    std::vector<DefenseLog> _logs;
    const int MAX_LOGS = 20;  // 最多保留20条记录
};

#endif // __DEFENSE_LOG_SYSTEM_H__
