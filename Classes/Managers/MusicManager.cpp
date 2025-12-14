/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MusicManager.cpp
 * File Function: 音乐管理器实现
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "MusicManager.h"
#include "cocos2d.h"

using namespace cocos2d;

MusicManager& MusicManager::getInstance()
{
    static MusicManager instance;
    return instance;
}

void MusicManager::initialize()
{
    // 初始化音频引擎（如果需要的话）
    // AudioEngine 会自动初始化，这里可以做一些预加载工作
}

std::string MusicManager::getMusicPath(MusicType type) const
{
    switch (type)
    {
        case MusicType::BATTLE_GOING:
            return "audio/background/Battle_Going.mp3";
        case MusicType::BATTLE_LOSE:
            return "audio/background/Battle_Lose.mp3";
        case MusicType::BATTLE_PREPARING:
            return "audio/background/Battle_Preparing.mp3";
        case MusicType::BATTLE_WIN:
            return "audio/background/Battle_Win.mp3";
        default:
            return "audio/background/Battle_Preparing.mp3";
    }
}

void MusicManager::playMusic(MusicType type, bool loop)
{
    // 如果正在播放相同的音乐，不做处理
    if (_currentAudioID != AudioEngine::INVALID_AUDIO_ID && _currentMusicType == type)
    {
        return;
    }

    // 停止当前播放的音乐
    stopMusic();

    // 播放新音乐
    std::string path = getMusicPath(type);
    _currentAudioID = AudioEngine::play2d(path, loop, _muted ? 0.0f : _volume);
    _currentMusicType = type;

    // 设置音频结束回调
    if (_currentAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::setFinishCallback(_currentAudioID, [this](int id, const std::string& filePath) {
            if (id == _currentAudioID)
            {
                _currentAudioID = AudioEngine::INVALID_AUDIO_ID;
            }
        });
    }
}

void MusicManager::stopMusic()
{
    if (_currentAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::stop(_currentAudioID);
        _currentAudioID = AudioEngine::INVALID_AUDIO_ID;
    }
}

void MusicManager::pauseMusic()
{
    if (_currentAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::pause(_currentAudioID);
    }
}

void MusicManager::resumeMusic()
{
    if (_currentAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::resume(_currentAudioID);
    }
}

void MusicManager::setVolume(float volume)
{
    _volume = std::max(0.0f, std::min(1.0f, volume));

    if (_currentAudioID != AudioEngine::INVALID_AUDIO_ID && !_muted)
    {
        AudioEngine::setVolume(_currentAudioID, _volume);
    }
}

float MusicManager::getVolume() const
{
    return _volume;
}

void MusicManager::setMuted(bool muted)
{
    _muted = muted;

    if (_currentAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::setVolume(_currentAudioID, _muted ? 0.0f : _volume);
    }
}

bool MusicManager::isMuted() const
{
    return _muted;
}

MusicType MusicManager::getCurrentMusicType() const
{
    return _currentMusicType;
}

bool MusicManager::isPlaying() const
{
    if (_currentAudioID == AudioEngine::INVALID_AUDIO_ID)
    {
        return false;
    }

    return AudioEngine::getState(_currentAudioID) == AudioEngine::AudioState::PLAYING;
}
