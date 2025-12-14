/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ReplaySystem.h
 * File Function: 回放系统 - 负责游戏的回放录制和回放
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __REPLAY_SYSTEM_H__
#define __REPLAY_SYSTEM_H__

#include "cocos2d.h"
#include "Unit/unit.h"
#include <string>
#include <vector>
#include <functional>
#include <sstream>

/**
 * @enum ReplayEventType
 * @brief 战斗回放事件类型
 */
enum class ReplayEventType {
    DEPLOY_UNIT,    // 部署士兵
    END_BATTLE      // 结束战斗
};

/**
 * @struct ReplayEvent
 * @brief 单个回放事件
 */
struct ReplayEvent {
    unsigned int frameIndex; // 事件发生的帧数（相对于战斗开始）
    ReplayEventType type;   // 事件类型
    int unitType;           // 士兵类型 (如果是部署事件)
    float x;                // X坐标
    float y;                // Y坐标
    
    std::string serialize() const;
    static ReplayEvent deserialize(const std::string& data);
};

/**
 * @struct ReplayData
 * @brief 完整的回放数据
 */
struct ReplayData {
    std::string enemyUserId;        // 敌方ID
    std::string enemyGameDataJson;  // 敌方基地数据快照 (JSON)
    unsigned int randomSeed;        // 随机种子
    std::vector<ReplayEvent> events;// 事件列表
    
    std::string serialize() const;
    static ReplayData deserialize(const std::string& data);
};

/**
 * @class ReplaySystem
 * @brief 战斗回放系统
 * 
 * 负责记录战斗过程中的关键事件，并能够序列化/反序列化，
 * 以及在回放模式下重现这些事件。
 */
class ReplaySystem {
public:
    static ReplaySystem& getInstance();
    
    // ==================== 录制功能 ====================
    
    /**
     * @brief 开始录制
     * @param enemyUserId 敌方ID
     * @param enemyGameDataJson 敌方基地数据快照
     * @param seed 随机种子
     */
    void startRecording(const std::string& enemyUserId, const std::string& enemyGameDataJson, unsigned int seed);
    
    /**
     * @brief 记录部署士兵事件
     */
    void recordDeployUnit(unsigned int frameIndex, UnitType unitType, const cocos2d::Vec2& position);
    
    /**
     * @brief 记录战斗结束事件
     */
    void recordEndBattle(unsigned int frameIndex);
    
    /**
     * @brief 停止录制并获取序列化数据
     */
    std::string stopRecording();
    
    /**
     * @brief 获取当前录制的数据
     */
    const ReplayData& getCurrentReplayData() const { return _currentReplayData; }
    
    // ==================== 回放功能 ====================
    
    /**
     * @brief 加载回放数据并准备回放
     */
    void loadReplay(const std::string& replayDataStr);
    
    /**
     * @brief 更新回放逻辑（在 fixedUpdate 中调用）
     */
    void updateFrame(unsigned int currentFrame);
    
    /**
     * @brief 设置部署士兵的回调
     */
    void setDeployUnitCallback(std::function<void(UnitType, const cocos2d::Vec2&)> callback);
    
    /**
     * @brief 设置结束战斗的回调
     */
    void setEndBattleCallback(std::function<void()> callback);
    
    /**
     * @brief 是否正在回放
     */
    bool isReplaying() const { return _isReplaying; }
    
    /**
     * @brief 是否正在录制
     */
    bool isRecording() const { return _isRecording; }
    
    /**
     * @brief 获取回放中的敌方ID
     */
    std::string getReplayEnemyUserId() const { return _currentReplayData.enemyUserId; }
    
    /**
     * @brief 获取回放中的随机种子
     */
    unsigned int getReplaySeed() const { return _currentReplayData.randomSeed; }

    /**
     * @brief 获取回放中的敌方基地数据快照
     */
    std::string getReplayEnemyGameDataJson() const { return _currentReplayData.enemyGameDataJson; }

    /**
     * @brief 重置系统状态
     */
    void reset();
    
private:
    ReplaySystem() = default;
    ~ReplaySystem() = default;
    ReplaySystem(const ReplaySystem&) = delete;
    ReplaySystem& operator=(const ReplaySystem&) = delete;
    
    bool _isRecording = false;
    bool _isReplaying = false;
    
    ReplayData _currentReplayData;
    size_t _nextEventIndex = 0;
    
    std::function<void(UnitType, const cocos2d::Vec2&)> _deployUnitCallback;
    std::function<void()> _endBattleCallback;
};

#endif // __REPLAY_SYSTEM_H__
