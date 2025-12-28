/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AudioHelper.h
 * File Function: 音频助手工具类
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#ifndef AUDIO_HELPER_H_
#define AUDIO_HELPER_H_

#include <functional>

#include "AudioManager.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

/**
 * @class AudioHelper
 * @brief 音频助手工具类
 *
 * 提供便捷的静态方法用于在UI组件上绑定音效。
 */
class AudioHelper {
 public:
  /**
   * @brief 为按钮添加点击音效
   * @param button 按钮指针
   * @param effect_id 音效类型，默认为按钮点击音效
   *
   * 注意：此方法会覆盖按钮已有的触摸事件监听器。
   * 如需保留原有逻辑，请使用 AddButtonClickSoundWithCallback。
   *
   * 使用示例：
   * @code
   * auto button = ui::Button::create(...);
   * AudioHelper::AddButtonClickSound(button);
   * @endcode
   */
  static void AddButtonClickSound(
      cocos2d::ui::Button* button,
      SoundEffectId effect_id = SoundEffectId::kUiButtonClick);

  /**
   * @brief 为按钮添加点击音效，并设置点击回调
   * @param button 按钮指针
   * @param effect_id 音效类型
   * @param on_click 点击时的回调函数
   *
   * 使用示例：
   * @code
   * AudioHelper::AddButtonClickSoundWithCallback(button,
   *     SoundEffectId::kUiButtonClick,
   *     [](Ref* sender) { CCLOG("Button clicked!"); });
   * @endcode
   */
  static void AddButtonClickSoundWithCallback(
      cocos2d::ui::Button* button,
      SoundEffectId effect_id,
      const std::function<void(cocos2d::Ref*)>& on_click);

  /**
   * @brief 为多个按钮批量添加点击音效
   * @param buttons 按钮列表
   * @param effect_id 音效类型
   */
  static void AddButtonClickSounds(
      const std::vector<cocos2d::ui::Button*>& buttons,
      SoundEffectId effect_id = SoundEffectId::kUiButtonClick);

  /**
   * @brief 播放按钮点击音效
   *
   * 可以在任何地方直接调用以播放点击音效。
   */
  static void PlayButtonClick();

  /**
   * @brief 播放资源收集音效
   * @param resource_type 资源类型字符串 ("gold", "elixir", "gem")
   */
  static void PlayResourceCollect(const std::string& resource_type);

 private:
  AudioHelper() = delete;  // 禁止实例化
};

#endif  // AUDIO_HELPER_H_
