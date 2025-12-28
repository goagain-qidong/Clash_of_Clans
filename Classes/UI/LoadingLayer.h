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
 * 支持音效播放完成后的回调机制。
 *
 * 使用示例：
 * @code
 * auto loading = LoadingLayer::create();
 * this->addChild(loading, 1000);
 * loading->show([](){ Director::getInstance()->replaceScene(...); });
 * @endcode
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
  virtual bool init() override;

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
  /// 加载完成回调
  std::function<void()> on_complete_callback_;

  /// 背景精灵
  cocos2d::Sprite* background_sprite_ = nullptr;

  /// 加载提示文本
  cocos2d::Label* loading_label_ = nullptr;

  /// 当前进度（0.0 ~ 100.0）
  float current_progress_ = 0.0f;

  /// 目标进度
  float target_progress_ = 0.0f;

  /// 是否正在加载
  bool is_loading_ = false;

  /// 加载阶段（0: supercell_logo, 1: loading_enter, 2: 完成）
  int loading_stage_ = 0;

  /// 进度条宽度
  float bar_width_ = 0.0f;

  /// 进度条高度
  float bar_height_ = 0.0f;

  // ==================== 私有方法 ====================

  /**
   * @brief 创建背景
   */
  void createBackground();

  /**
   * @brief 创建进度条
   */
  void createProgressBar();

  /**
   * @brief 创建加载文本
   */
  void createLoadingText();

  /**
   * @brief 创建装饰动画
   */
  void createDecorations();

  /**
   * @brief 开始播放音效序列
   */
  void startAudioSequence();

  /**
   * @brief 播放第一个音效（supercell_logo）
   */
  void playFirstAudio();

  /**
   * @brief 播放第二个音效（loading_enter）
   */
  void playSecondAudio();

  /**
   * @brief 完成加载
   */
  void finishLoading();

  /**
   * @brief 更新进度条
   * @param dt 帧间隔时间
   */
  void updateProgress(float dt);

  /**
   * @brief 更新加载文本动画
   * @param dt 帧间隔时间
   */
  void updateLoadingText(float dt);

  // ==================== 常量 ====================

  /// 加载背景图片路径
  static constexpr const char* kLoadingBackgroundPath =
      "loading/loading_picture_christmas.jpg";

  /// 进度条更新速度（每秒百分比）
  static constexpr float kProgressSpeed = 25.0f;

  /// 文本动画间隔（秒）
  static constexpr float kTextAnimInterval = 0.3f;
};

#endif  // LOADING_LAYER_H_
