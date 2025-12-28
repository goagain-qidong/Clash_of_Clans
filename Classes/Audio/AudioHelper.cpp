/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AudioHelper.cpp
 * File Function: 音频助手工具类实现
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#include "AudioHelper.h"

void AudioHelper::AddButtonClickSound(cocos2d::ui::Button* button,
                                      SoundEffectId effect_id) {
  if (!button) {
    return;
  }

  // 设置新的回调，播放音效
  // 注意：这会覆盖原有的回调，所以应该在设置其他回调之前调用
  // 或者使用包装模式（需要用户自己在回调中处理）
  button->addTouchEventListener(
      [effect_id](cocos2d::Ref* sender,
                  cocos2d::ui::Widget::TouchEventType type) {
        if (type == cocos2d::ui::Widget::TouchEventType::ENDED) {
          // 播放点击音效
          AudioManager::GetInstance().PlayEffect(effect_id);
        }
      });
}

void AudioHelper::AddButtonClickSoundWithCallback(
    cocos2d::ui::Button* button,
    SoundEffectId effect_id,
    const std::function<void(cocos2d::Ref*)>& on_click) {
  if (!button) {
    return;
  }

  button->addTouchEventListener(
      [effect_id, on_click](cocos2d::Ref* sender,
                            cocos2d::ui::Widget::TouchEventType type) {
        if (type == cocos2d::ui::Widget::TouchEventType::ENDED) {
          // 播放点击音效
          AudioManager::GetInstance().PlayEffect(effect_id);
          // 调用用户回调
          if (on_click) {
            on_click(sender);
          }
        }
      });
}

void AudioHelper::AddButtonClickSounds(
    const std::vector<cocos2d::ui::Button*>& buttons,
    SoundEffectId effect_id) {
  for (auto* button : buttons) {
    AddButtonClickSound(button, effect_id);
  }
}

void AudioHelper::PlayButtonClick() {
  AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
}

void AudioHelper::PlayResourceCollect(const std::string& resource_type) {
  if (resource_type == "gold" || resource_type == "Gold") {
    AudioManager::GetInstance().PlayEffect(SoundEffectId::kResourceGoldCollect);
  } else if (resource_type == "elixir" || resource_type == "Elixir") {
    AudioManager::GetInstance().PlayEffect(
        SoundEffectId::kResourceElixirCollect);
  } else if (resource_type == "gem" || resource_type == "Gem" ||
             resource_type == "diamond" || resource_type == "Diamond") {
    AudioManager::GetInstance().PlayEffect(SoundEffectId::kResourceGemCollect);
  }
}
