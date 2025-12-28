/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     LoadingLayer.cpp
 * File Function: 加载界面层实现
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#include "LoadingLayer.h"

#include "Audio/AudioManager.h"
#include "audio/include/AudioEngine.h"

USING_NS_CC;

LoadingLayer* LoadingLayer::create() {
  LoadingLayer* layer = new (std::nothrow) LoadingLayer();
  if (layer && layer->init()) {
    layer->autorelease();
    return layer;
  }
  CC_SAFE_DELETE(layer);
  return nullptr;
}

bool LoadingLayer::init() {
  if (!Layer::init()) {
    return false;
  }

  // 创建各个UI组件
  createBackground();
  createProgressBar();
  createLoadingText();
  createDecorations();

  // 默认隐藏
  this->setVisible(false);

  return true;
}

void LoadingLayer::createBackground() {
  auto visible_size = Director::getInstance()->getVisibleSize();
  auto origin = Director::getInstance()->getVisibleOrigin();

  // 创建半透明黑色遮罩层
  auto mask = LayerColor::create(Color4B(0, 0, 0, 255));
  this->addChild(mask, 0);

  // 创建背景图片
  background_sprite_ = Sprite::create(kLoadingBackgroundPath);
  if (background_sprite_) {
    // 计算缩放比例，使图片填满屏幕
    float scale_x = visible_size.width / background_sprite_->getContentSize().width;
    float scale_y = visible_size.height / background_sprite_->getContentSize().height;
    float scale = std::max(scale_x, scale_y);

    background_sprite_->setScale(scale);
    background_sprite_->setPosition(
        Vec2(origin.x + visible_size.width / 2,
             origin.y + visible_size.height / 2));

    // 初始透明度为0，用于淡入动画
    background_sprite_->setOpacity(0);
    this->addChild(background_sprite_, 1);
  } else {
    CCLOG("⚠️ 加载背景图片失败: %s", kLoadingBackgroundPath);
  }
}

void LoadingLayer::createProgressBar() {
  auto visible_size = Director::getInstance()->getVisibleSize();
  auto origin = Director::getInstance()->getVisibleOrigin();

  // 进度条位置（屏幕底部偏上）
  float bar_y = origin.y + visible_size.height * 0.15f;
  bar_width_ = visible_size.width * 0.6f;
  bar_height_ = 20.0f;

  // 创建进度条背景（深色圆角矩形）
  auto bar_bg = DrawNode::create();
  Vec2 bar_center(origin.x + visible_size.width / 2, bar_y);

  // 绘制圆角矩形背景
  bar_bg->drawSolidRect(
      Vec2(bar_center.x - bar_width_ / 2, bar_center.y - bar_height_ / 2),
      Vec2(bar_center.x + bar_width_ / 2, bar_center.y + bar_height_ / 2),
      Color4F(0.1f, 0.1f, 0.1f, 0.8f));
  this->addChild(bar_bg, 2);

  // 创建进度条边框
  auto bar_border = DrawNode::create();
  bar_border->drawRect(
      Vec2(bar_center.x - bar_width_ / 2 - 2, bar_center.y - bar_height_ / 2 - 2),
      Vec2(bar_center.x + bar_width_ / 2 + 2, bar_center.y + bar_height_ / 2 + 2),
      Color4F(0.8f, 0.6f, 0.2f, 1.0f));
  this->addChild(bar_border, 3);

  // 创建进度条填充节点
  auto progress_fill = DrawNode::create();
  progress_fill->setName("progress_fill");
  progress_fill->setPosition(Vec2(bar_center.x - bar_width_ / 2 + 2, bar_center.y));
  this->addChild(progress_fill, 4);
}

void LoadingLayer::createLoadingText() {
  auto visible_size = Director::getInstance()->getVisibleSize();
  auto origin = Director::getInstance()->getVisibleOrigin();

  // 加载提示文本
  loading_label_ = Label::createWithSystemFont("加载中", "Arial", 28);
  loading_label_->setPosition(
      Vec2(origin.x + visible_size.width / 2,
           origin.y + visible_size.height * 0.08f));
  loading_label_->setTextColor(Color4B(255, 220, 150, 255));
  loading_label_->enableOutline(Color4B(0, 0, 0, 200), 2);
  this->addChild(loading_label_, 10);

  // 版本/提示文本
  auto hint_label = Label::createWithSystemFont(
      "首次加载可能需要较长时间，请耐心等待...", "Arial", 16);
  hint_label->setPosition(
      Vec2(origin.x + visible_size.width / 2,
           origin.y + visible_size.height * 0.04f));
  hint_label->setTextColor(Color4B(180, 180, 180, 200));
  this->addChild(hint_label, 10);
}

void LoadingLayer::createDecorations() {
  auto visible_size = Director::getInstance()->getVisibleSize();
  auto origin = Director::getInstance()->getVisibleOrigin();

  // 创建闪烁星星装饰效果（替代粒子系统）
  for (int i = 0; i < 20; ++i) {
    auto star = DrawNode::create();
    float x = origin.x + (rand() % static_cast<int>(visible_size.width));
    float y = origin.y + visible_size.height * 0.3f +
              (rand() % static_cast<int>(visible_size.height * 0.6f));
    float size = 2.0f + (rand() % 3);

    star->drawDot(Vec2::ZERO, size, Color4F(1.0f, 1.0f, 1.0f, 0.8f));
    star->setPosition(Vec2(x, y));
    star->setOpacity(0);
    this->addChild(star, 6);

    // 随机延迟的闪烁动画
    float delay = (rand() % 100) / 100.0f * 2.0f;
    auto fade_in = FadeIn::create(0.5f + (rand() % 50) / 100.0f);
    auto fade_out = FadeOut::create(0.5f + (rand() % 50) / 100.0f);
    auto blink = Sequence::create(
        DelayTime::create(delay),
        fade_in,
        DelayTime::create(0.2f),
        fade_out,
        nullptr);
    star->runAction(RepeatForever::create(blink));
  }

  // 创建顶部渐变遮罩（让背景图更好地融入）
  auto top_gradient = LayerColor::create(Color4B(0, 0, 0, 150));
  top_gradient->setContentSize(Size(visible_size.width, visible_size.height * 0.15f));
  top_gradient->setPosition(Vec2(0, visible_size.height * 0.85f));
  this->addChild(top_gradient, 7);

  // 创建底部渐变遮罩
  auto bottom_gradient = LayerColor::create(Color4B(0, 0, 0, 180));
  bottom_gradient->setContentSize(Size(visible_size.width, visible_size.height * 0.25f));
  bottom_gradient->setPosition(Vec2(0, 0));
  this->addChild(bottom_gradient, 7);
}

void LoadingLayer::show(const std::function<void()>& on_complete) {
  on_complete_callback_ = on_complete;
  is_loading_ = true;
  loading_stage_ = 0;
  current_progress_ = 0.0f;
  target_progress_ = 0.0f;

  this->setVisible(true);

  // 背景淡入动画
  if (background_sprite_) {
    background_sprite_->runAction(FadeIn::create(0.5f));

    // 背景轻微缩放动画（呼吸效果）
    auto scale_up = ScaleTo::create(8.0f, background_sprite_->getScale() * 1.05f);
    auto scale_down = ScaleTo::create(8.0f, background_sprite_->getScale());
    auto breath = Sequence::create(scale_up, scale_down, nullptr);
    background_sprite_->runAction(RepeatForever::create(breath));
  }

  // 启动进度条更新
  this->schedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateProgress), 1.0f / 60.0f);

  // 启动文本动画
  this->schedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateLoadingText), kTextAnimInterval);

  // 开始音效序列
  startAudioSequence();
}

void LoadingLayer::hide() {
  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateProgress));
  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateLoadingText));

  // 淡出动画
  auto fade_out = FadeOut::create(0.3f);
  auto remove = CallFunc::create([this]() {
    this->removeFromParent();
  });
  this->runAction(Sequence::create(fade_out, remove, nullptr));
}

void LoadingLayer::startAudioSequence() {
  // 设置初始目标进度
  target_progress_ = 30.0f;
  loading_stage_ = 0;

  // 播放第一个音效
  playFirstAudio();
}

void LoadingLayer::playFirstAudio() {
  int audio_id = AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiSupercellLogo);

  if (audio_id != AudioManager::kInvalidAudioId) {
    // 设置播放完成回调
    cocos2d::AudioEngine::setFinishCallback(
        audio_id, [this](int id, const std::string& file) {
          // 进入第二阶段
          loading_stage_ = 1;
          target_progress_ = 70.0f;
          playSecondAudio();
        });
  } else {
    // 音效播放失败，直接进入下一阶段
    CCLOG("⚠️ supercell_logo 音效播放失败");
    loading_stage_ = 1;
    target_progress_ = 70.0f;
    playSecondAudio();
  }
}

void LoadingLayer::playSecondAudio() {
  int audio_id = AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiLoadingEnter);

  if (audio_id != AudioManager::kInvalidAudioId) {
    // 设置播放完成回调
    cocos2d::AudioEngine::setFinishCallback(
        audio_id, [this](int id, const std::string& file) {
          // 进入完成阶段
          loading_stage_ = 2;
          target_progress_ = 100.0f;
        });
  } else {
    // 音效播放失败，直接完成
    CCLOG("⚠️ loading_enter 音效播放失败");
    loading_stage_ = 2;
    target_progress_ = 100.0f;
  }
}

void LoadingLayer::finishLoading() {
  is_loading_ = false;

  // 停止定时器
  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateProgress));
  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateLoadingText));

  // 短暂延迟后执行回调
  this->scheduleOnce(
      [this](float dt) {
        if (on_complete_callback_) {
          on_complete_callback_();
        }
      },
      0.3f, "finish_loading");
}

void LoadingLayer::updateProgress(float dt) {
  if (!is_loading_) {
    return;
  }

  // 平滑更新进度
  if (current_progress_ < target_progress_) {
    float speed = kProgressSpeed;
    // 接近目标时减速
    if (target_progress_ - current_progress_ < 10.0f) {
      speed = kProgressSpeed * 0.5f;
    }
    current_progress_ += speed * dt;
    if (current_progress_ > target_progress_) {
      current_progress_ = target_progress_;
    }
  }

  // 更新进度条显示
  auto progress_fill = this->getChildByName<DrawNode*>("progress_fill");
  if (progress_fill) {
    progress_fill->clear();

    // 计算当前进度对应的宽度
    float fill_width = (bar_width_ - 4) * (current_progress_ / 100.0f);
    float fill_height = bar_height_ - 4;

    if (fill_width > 0) {
      // 绘制渐变填充
      Color4F start_color(0.2f, 0.8f, 0.3f, 1.0f);  // 绿色
      Color4F end_color(0.8f, 0.9f, 0.2f, 1.0f);    // 黄绿色

      // 简单渐变效果
      float t = current_progress_ / 100.0f;
      Color4F fill_color(
          start_color.r + (end_color.r - start_color.r) * t,
          start_color.g + (end_color.g - start_color.g) * t,
          start_color.b + (end_color.b - start_color.b) * t,
          1.0f);

      progress_fill->drawSolidRect(
          Vec2(0, -fill_height / 2),
          Vec2(fill_width, fill_height / 2),
          fill_color);

      // 添加高光效果
      progress_fill->drawSolidRect(
          Vec2(0, fill_height / 4),
          Vec2(fill_width, fill_height / 2),
          Color4F(1.0f, 1.0f, 1.0f, 0.2f));
    }
  }

  // 检查是否完成
  if (current_progress_ >= 100.0f && loading_stage_ >= 2) {
    finishLoading();
  }
}

void LoadingLayer::updateLoadingText(float dt) {
  static int dot_count = 0;
  dot_count = (dot_count + 1) % 4;

  std::string dots = "";
  for (int i = 0; i < dot_count; ++i) {
    dots += ".";
  }

  if (loading_label_) {
    std::string text = "加载中" + dots;

    // 根据加载阶段显示不同提示
    if (loading_stage_ == 0) {
      text = "正在初始化" + dots;
    } else if (loading_stage_ == 1) {
      text = "正在加载资源" + dots;
    } else if (loading_stage_ >= 2) {
      text = "即将进入游戏" + dots;
    }

    loading_label_->setString(text);
  }
}
