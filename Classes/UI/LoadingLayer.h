/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     LoadingLayer.h
 * File Function: 加载界面层头文件
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#ifndef LOADING_LAYER_H_
#define LOADING_LAYER_H_

#include <functional>
#include <string>

#include "cocos2d.h"
#include "ui/CocosGUI.h"

/**
 * @class LoadingLayer
 * @brief 加载界面层
 *
 * 用于在场景切换时显示加载动画、进度条和背景图片。
 * 进度条会持续平滑前进，音效播放完成时加速进度。
 */
class LoadingLayer : public cocos2d::Layer {
 public:
  /**
   * @brief 创建加载层实例
   * @return LoadingLayer* 加载层指针
   */
  static LoadingLayer* create();

  /**
   * @brief 初始化
   * @return bool 是否成功
   */
  bool init() override;

  /**
   * @brief 显示加载界面并开始加载流程
   * @param on_complete 加载完成后的回调函数
   *
   * 加载流程：
   * 1. 显示加载背景和进度条
   * 2. 播放 supercell_logo 音效
   * 3. 播放 loading_enter 音效
   * 4. 进度条完成后调用回调
   */
  void show(const std::function<void()>& on_complete);

  /**
   * @brief 立即隐藏加载界面
   */
  void hide();

 private:
  // ==================== 私有方法 ====================

  void createBackground();
  void createProgressBar();
  void createLoadingText();
  void createDecorations();
  void startAudioSequence();
  void playFirstAudio();
  void playSecondAudio();
  void finishLoading();
  void updateProgress(float dt);
  void updateLoadingText(float dt);

  // ==================== 成员变量 ====================

  std::function<void()> on_complete_callback_;
  cocos2d::Sprite* background_sprite_ = nullptr;
  cocos2d::Label* loading_label_ = nullptr;
  float current_progress_ = 0.0f;
  bool is_loading_ = false;
  int loading_stage_ = 0;
  float bar_width_ = 0.0f;
  float bar_height_ = 0.0f;

  // 音效完成标记
  bool first_audio_done_ = false;
  bool second_audio_done_ = false;

  // ==================== 常量 ====================

  /// 加载背景图片路径
  static constexpr const char* kLoadingBackgroundPath =
      "loading/loading_picture_christmas.jpg";

  // 基础进度速度（每秒百分比）- 音效未完成时的缓慢速度
  static constexpr float kBaseProgressSpeed = 25.0f;

  // 加速进度速度（每秒百分比）- 音效完成后的快速速度
  static constexpr float kFastProgressSpeed = 200.0f;

  /// 文本动画间隔（秒）
  static constexpr float kTextAnimInterval = 0.2f;

  /// 完成后进入游戏的延迟（秒）
  static constexpr float kFinishDelay = 0.05f;
};

#endif  // LOADING_LAYER_H_
