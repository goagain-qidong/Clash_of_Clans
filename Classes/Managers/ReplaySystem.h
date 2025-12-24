/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ReplaySystem.h
 * File Function: 回放系统 - 负责游戏的回放录制和回放
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef REPLAY_SYSTEM_H_
#define REPLAY_SYSTEM_H_

#include "cocos2d.h"
#include "Unit/UnitTypes.h"

#include <functional>
#include <sstream>
#include <string>
#include <vector>

/**
 * @enum ReplayEventType
 * @brief 战斗回放事件类型
 */
enum class ReplayEventType {
    DEPLOY_UNIT,  ///< 部署士兵
    END_BATTLE    ///< 结束战斗
};

/**
 * @struct ReplayEvent
 * @brief 单个回放事件
 */
struct ReplayEvent {
    unsigned int frameIndex;  ///< 事件发生的帧数
    ReplayEventType type;     ///< 事件类型
    int unitType;             ///< 士兵类型
    float x;                  ///< X坐标
    float y;                  ///< Y坐标

    /** @brief 序列化事件 */
    std::string serialize() const;

    /**
     * @brief 反序列化事件
     * @param data 序列化数据
     * @return ReplayEvent 事件对象
     */
    static ReplayEvent deserialize(const std::string& data);
};

/**
 * @struct ReplayData
 * @brief 完整的回放数据
 */
struct ReplayData {
    std::string enemyUserId;         ///< 敌方ID
    std::string enemyGameDataJson;   ///< 敌方基地数据快照
    unsigned int randomSeed;         ///< 随机种子
    std::vector<ReplayEvent> events; ///< 事件列表

    /** @brief 序列化回放数据 */
    std::string serialize() const;

    /**
     * @brief 反序列化回放数据
     * @param data 序列化数据
     * @return ReplayData 回放数据对象
     */
    static ReplayData deserialize(const std::string& data);
};

/**
 * @class ReplaySystem
 * @brief 战斗回放系统（单例）
 *
 * 负责记录战斗过程中的关键事件，并能够序列化/反序列化，
 * 以及在回放模式下重现这些事件。
 */
class ReplaySystem {
public:
    /**
     * @brief 获取单例实例
     * @return ReplaySystem& 单例引用
     */
    static ReplaySystem& getInstance();

    /**
     * @brief 开始录制
     * @param enemyUserId 敌方ID
     * @param enemyGameDataJson 敌方基地数据快照
     * @param seed 随机种子
     */
    void startRecording(const std::string& enemyUserId, const std::string& enemyGameDataJson, unsigned int seed);

    /**
     * @brief 记录部署士兵事件
     * @param frameIndex 帧索引
     * @param unitType 士兵类型
     * @param position 部署位置
     */
    void recordDeployUnit(unsigned int frameIndex, UnitType unitType, const cocos2d::Vec2& position);

    /**
     * @brief 记录战斗结束事件
     * @param frameIndex 帧索引
     */
    void recordEndBattle(unsigned int frameIndex);

    /**
     * @brief 停止录制并获取序列化数据
     * @return std::string 序列化的回放数据
     */
    std::string stopRecording();

    /** @brief 获取当前录制的数据 */
    const ReplayData& getCurrentReplayData() const { return _currentReplayData; }

    /**
     * @brief 加载回放数据并准备回放
     * @param replayDataStr 序列化的回放数据
     */
    void loadReplay(const std::string& replayDataStr);

    /**
     * @brief 更新回放逻辑
     * @param currentFrame 当前帧
     */
    void updateFrame(unsigned int currentFrame);

    /** @brief 设置部署士兵的回调 */
    void setDeployUnitCallback(std::function<void(UnitType, const cocos2d::Vec2&)> callback);

    /** @brief 设置结束战斗的回调 */
    void setEndBattleCallback(std::function<void()> callback);

    /** @brief 是否正在回放 */
    bool isReplaying() const { return _isReplaying; }

    /** @brief 是否正在录制 */
    bool isRecording() const { return _isRecording; }

    /** @brief 获取回放中的敌方ID */
    std::string getReplayEnemyUserId() const { return _currentReplayData.enemyUserId; }

    /** @brief 获取回放中的随机种子 */
    unsigned int getReplaySeed() const { return _currentReplayData.randomSeed; }

    /** @brief 获取回放中的敌方基地数据快照 */
    std::string getReplayEnemyGameDataJson() const { return _currentReplayData.enemyGameDataJson; }

    /** @brief 重置系统状态 */
    void reset();

private:
    ReplaySystem() = default;
    ~ReplaySystem() = default;
    ReplaySystem(const ReplaySystem&) = delete;
    ReplaySystem& operator=(const ReplaySystem&) = delete;

    bool _isRecording = false;  ///< 是否正在录制
    bool _isReplaying = false;  ///< 是否正在回放

    ReplayData _currentReplayData;  ///< 当前回放数据
    size_t _nextEventIndex = 0;     ///< 下一个事件索引

    std::function<void(UnitType, const cocos2d::Vec2&)> _deployUnitCallback;  ///< 部署回调
    std::function<void()> _endBattleCallback;  ///< 结束回调
};

#endif  // REPLAY_SYSTEM_H_
