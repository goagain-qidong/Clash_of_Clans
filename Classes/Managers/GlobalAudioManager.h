/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GlobalAudioManager.h
 * File Function: 全局音频管理器头文件
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "cocos2d.h"
#include "audio/include/AudioEngine.h"
#include <vector>

/**
 * @class GlobalAudioManager
 * @brief 全局音频管理器，统一管理所有音乐和音效的音量
 */
class GlobalAudioManager
{
public:
    static GlobalAudioManager& getInstance();
    
    // ==================== 音乐管理 ====================
    
    /**
     * @brief 播放背景音乐（循环）
     * @param filename 音乐文件路径
     * @return 音频ID
     */
    int playMusic(const std::string& filename);
    
    /**
     * @brief 停止当前音乐
     */
    void stopMusic();
    
    /**
     * @brief 设置音乐音量（0.0 ~ 1.0）
     */
    void setMusicVolume(float volume);
    
    /**
     * @brief 获取音乐音量
     */
    float getMusicVolume() const { return _musicVolume; }
    
    // ==================== 音效管理 ====================
    
    /**
     * @brief 播放音效
     * @param filename 音效文件路径
     * @return 音效ID
     */
    int playEffect(const std::string& filename);
    
    /**
     * @brief 设置音效音量（0.0 ~ 1.0）
     */
    void setEffectVolume(float volume);
    
    /**
     * @brief 获取音效音量
     */
    float getEffectVolume() const { return _effectVolume; }
    
    // ==================== 保存/加载设置 ====================
    
    void loadSettings();
    void saveSettings();
    
private:
    GlobalAudioManager();
    ~GlobalAudioManager() = default;
    
    static GlobalAudioManager* _instance;
    
    float _musicVolume = 1.0f;    // 音乐音量
    float _effectVolume = 1.0f;   // 音效音量
    
    int _currentMusicID = -1;     // 当前播放的音乐ID
    std::vector<int> _effectIDs;  // 所有正在播放的音效ID
};
