/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MusicManager.h
 * File Function: 音乐管理器头文件
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>

#include "audio/include/AudioEngine.h"

/**
 * @enum MusicType
 * @brief 音乐类型枚举
 */
enum class MusicType
{
    BATTLE_GOING,      ///< 战斗进行中
    BATTLE_LOSE,       ///< 战斗失败
    BATTLE_PREPARING,  ///< 战斗准备
    BATTLE_WIN         ///< 战斗胜利
};

/**
 * @class MusicManager
 * @brief 音乐管理器（单例）
 */
class MusicManager
{
public:
    /**
     * @brief 获取单例实例
     * @return MusicManager& 单例引用
     */
    static MusicManager& getInstance();

    /** @brief 初始化音乐管理器 */
    void initialize();

    /**
     * @brief 播放指定类型的背景音乐
     * @param type 音乐类型
     * @param loop 是否循环
     */
    void playMusic(MusicType type, bool loop = true);

    /** @brief 停止当前播放的音乐 */
    void stopMusic();

    /** @brief 暂停音乐 */
    void pauseMusic();

    /** @brief 恢复音乐 */
    void resumeMusic();

    /**
     * @brief 设置音量
     * @param volume 音量值 (0.0f - 1.0f)
     */
    void setVolume(float volume);

    /** @brief 获取当前音量 */
    float getVolume() const;

    /**
     * @brief 设置是否静音
     * @param muted 是否静音
     */
    void setMuted(bool muted);

    /** @brief 获取是否静音 */
    bool isMuted() const;

    /** @brief 获取当前播放的音乐类型 */
    MusicType getCurrentMusicType() const;

    /** @brief 是否正在播放 */
    bool isPlaying() const;

private:
    MusicManager() = default;
    MusicManager(const MusicManager&) = delete;
    MusicManager& operator=(const MusicManager&) = delete;

    std::string getMusicPath(MusicType type) const;

    int _currentAudioID = cocos2d::AudioEngine::INVALID_AUDIO_ID;  ///< 当前音频ID
    MusicType _currentMusicType = MusicType::BATTLE_PREPARING;     ///< 当前音乐类型
    float _volume = 1.0f;  ///< 音量
    bool _muted = false;   ///< 是否静音
};
