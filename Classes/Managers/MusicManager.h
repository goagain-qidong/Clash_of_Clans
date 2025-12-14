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

enum class MusicType
{
    BATTLE_GOING,      // 战斗进行中
    BATTLE_LOSE,       // 战斗失败
    BATTLE_PREPARING,  // 战斗准备
    BATTLE_WIN         // 战斗胜利
};

class MusicManager
{
public:
    static MusicManager& getInstance();

    // 初始化音乐管理器
    void initialize();

    // 播放指定类型的背景音乐
    void playMusic(MusicType type, bool loop = true);

    // 停止当前播放的音乐
    void stopMusic();

    // 暂停音乐
    void pauseMusic();

    // 恢复音乐
    void resumeMusic();

    // 设置音量 (0.0f - 1.0f)
    void setVolume(float volume);

    // 获取当前音量
    float getVolume() const;

    // 设置是否静音
    void setMuted(bool muted);

    // 获取是否静音
    bool isMuted() const;

    // 获取当前播放的音乐类型
    MusicType getCurrentMusicType() const;

    // 是否正在播放
    bool isPlaying() const;

private:
    MusicManager() = default;
    MusicManager(const MusicManager&) = delete;
    MusicManager& operator=(const MusicManager&) = delete;

    std::string getMusicPath(MusicType type) const;

    int _currentAudioID = cocos2d::AudioEngine::INVALID_AUDIO_ID;
    MusicType _currentMusicType = MusicType::BATTLE_PREPARING;
    float _volume = 1.0f;
    bool _muted = false;
};
