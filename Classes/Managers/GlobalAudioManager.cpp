/**
 * @file GlobalAudioManager.cpp
 * @brief 全局音频管理器实现
 */

#include "GlobalAudioManager.h"

USING_NS_CC;

GlobalAudioManager* GlobalAudioManager::_instance = nullptr;

GlobalAudioManager& GlobalAudioManager::getInstance()
{
    if (!_instance)
    {
        _instance = new (std::nothrow) GlobalAudioManager();
        _instance->loadSettings();
    }
    return *_instance;
}

GlobalAudioManager::GlobalAudioManager()
{
}

// ==================== 音乐管理 ====================

int GlobalAudioManager::playMusic(const std::string& filename)
{
    // 停止当前音乐
    if (_currentMusicID != -1)
    {
        cocos2d::AudioEngine::stop(_currentMusicID);
    }
    
    // 播放新音乐（循环播放）
    _currentMusicID = cocos2d::AudioEngine::play2d(filename, true, _musicVolume);
    
    CCLOG("🎵 Playing music: %s (ID: %d, Volume: %.2f)", filename.c_str(), _currentMusicID, _musicVolume);
    
    return _currentMusicID;
}

void GlobalAudioManager::stopMusic()
{
    if (_currentMusicID != -1)
    {
        cocos2d::AudioEngine::stop(_currentMusicID);
        _currentMusicID = -1;
    }
}

void GlobalAudioManager::setMusicVolume(float volume)
{
    _musicVolume = std::max(0.0f, std::min(1.0f, volume));
    
    // 如果有音乐正在播放，立即更新音量
    if (_currentMusicID != -1)
    {
        cocos2d::AudioEngine::setVolume(_currentMusicID, _musicVolume);
        CCLOG("🔊 Music volume updated: %.2f", _musicVolume);
    }
    
    saveSettings();
}

// ==================== 音效管理 ====================

int GlobalAudioManager::playEffect(const std::string& filename)
{
    int effectID = cocos2d::AudioEngine::play2d(filename, false, _effectVolume);
    
    if (effectID != cocos2d::AudioEngine::INVALID_AUDIO_ID)
    {
        _effectIDs.push_back(effectID);
        
        // 设置完成回调，播放完后从列表中移除
        cocos2d::AudioEngine::setFinishCallback(effectID, [this, effectID](int id, const std::string& file) {
            auto it = std::find(_effectIDs.begin(), _effectIDs.end(), effectID);
            if (it != _effectIDs.end())
            {
                _effectIDs.erase(it);
            }
        });
        
        CCLOG("🔔 Playing effect: %s (ID: %d, Volume: %.2f)", filename.c_str(), effectID, _effectVolume);
    }
    
    return effectID;
}

void GlobalAudioManager::setEffectVolume(float volume)
{
    _effectVolume = std::max(0.0f, std::min(1.0f, volume));
    
    // 更新所有正在播放的音效音量
    for (int effectID : _effectIDs)
    {
        if (cocos2d::AudioEngine::getState(effectID) == cocos2d::AudioEngine::AudioState::PLAYING)
        {
            cocos2d::AudioEngine::setVolume(effectID, _effectVolume);
        }
    }
    
    CCLOG("🔊 Effect volume updated: %.2f", _effectVolume);
    
    saveSettings();
}

// ==================== 保存/加载设置 ====================

void GlobalAudioManager::loadSettings()
{
    auto userDefault = UserDefault::getInstance();
    _musicVolume = userDefault->getFloatForKey("GlobalMusicVolume", 1.0f);
    _effectVolume = userDefault->getFloatForKey("GlobalEffectVolume", 1.0f);
    
    CCLOG("📂 Audio settings loaded: Music=%.2f, Effect=%.2f", _musicVolume, _effectVolume);
}

void GlobalAudioManager::saveSettings()
{
    auto userDefault = UserDefault::getInstance();
    userDefault->setFloatForKey("GlobalMusicVolume", _musicVolume);
    userDefault->setFloatForKey("GlobalEffectVolume", _effectVolume);
    userDefault->flush();
}
