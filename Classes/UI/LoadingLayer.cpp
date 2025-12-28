/************************************************************************
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

  createBackground();
  createProgressBar();
  createLoadingText();
  createDecorations();

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
    float scale_x = visible_size.width / background_sprite_->getContentSize().width;
    float scale_y = visible_size.height / background_sprite_->getContentSize().height;
    float scale = std::max(scale_x, scale_y);

    background_sprite_->setScale(scale);
    background_sprite_->setPosition(
        Vec2(origin.x + visible_size.width / 2,
             origin.y + visible_size.height / 2));
    background_sprite_->setOpacity(0);
    this->addChild(background_sprite_, 1);
  } else {
    CCLOG("⚠️ 加载背景图片失败: %s", kLoadingBackgroundPath);
  }
}

void LoadingLayer::createProgressBar() {
  auto visible_size = Director::getInstance()->getVisibleSize();
  auto origin = Director::getInstance()->getVisibleOrigin();

  float bar_y = origin.y + visible_size.height * 0.15f;
  bar_width_ = visible_size.width * 0.6f;
  bar_height_ = 20.0f;

  Vec2 bar_center(origin.x + visible_size.width / 2, bar_y);

  // 进度条背景
  auto bar_bg = DrawNode::create();
  bar_bg->drawSolidRect(
      Vec2(bar_center.x - bar_width_ / 2, bar_center.y - bar_height_ / 2),
      Vec2(bar_center.x + bar_width_ / 2, bar_center.y + bar_height_ / 2),
      Color4F(0.1f, 0.1f, 0.1f, 0.8f));
  this->addChild(bar_bg, 2);

  // 进度条边框
  auto bar_border = DrawNode::create();
  bar_border->drawRect(
      Vec2(bar_center.x - bar_width_ / 2 - 2, bar_center.y - bar_height_ / 2 - 2),
      Vec2(bar_center.x + bar_width_ / 2 + 2, bar_center.y + bar_height_ / 2 + 2),
      Color4F(0.8f, 0.6f, 0.2f, 1.0f));
  this->addChild(bar_border, 3);

  // 进度条填充节点
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

  // 提示文本
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

  // 创建闪烁星星装饰效果
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

    float delay = (rand() % 100) / 100.0f * 2.0f;
    auto fade_in = FadeIn::create(0.5f + (rand() % 50) / 100.0f);
    auto fade_out = FadeOut::create(0.5f + (rand() % 50) / 100.0f);
    auto blink = Sequence::create(
        DelayTime::create(delay), fade_in, DelayTime::create(0.2f), fade_out, nullptr);
    star->runAction(RepeatForever::create(blink));
  }

  // 顶部渐变遮罩
  auto top_gradient = LayerColor::create(Color4B(0, 0, 0, 150));
  top_gradient->setContentSize(Size(visible_size.width, visible_size.height * 0.15f));
  top_gradient->setPosition(Vec2(0, visible_size.height * 0.85f));
  this->addChild(top_gradient, 7);

  // 底部渐变遮罩
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
  first_audio_done_ = false;
  second_audio_done_ = false;

  this->setVisible(true);

  // 背景淡入和呼吸动画
  if (background_sprite_) {
    background_sprite_->runAction(FadeIn::create(0.25f));

    auto scale_up = ScaleTo::create(8.0f, background_sprite_->getScale() * 1.05f);
    auto scale_down = ScaleTo::create(8.0f, background_sprite_->getScale());
    background_sprite_->runAction(
        RepeatForever::create(Sequence::create(scale_up, scale_down, nullptr)));
  }

  this->schedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateProgress), 1.0f / 60.0f);
  this->schedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateLoadingText), kTextAnimInterval);

  startAudioSequence();
}

void LoadingLayer::hide() {
  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateProgress));
  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateLoadingText));

  auto fade_out = FadeOut::create(0.15f);
  auto remove = CallFunc::create([this]() { this->removeFromParent(); });
  this->runAction(Sequence::create(fade_out, remove, nullptr));
}

void LoadingLayer::startAudioSequence() {
  playFirstAudio();
}

void LoadingLayer::playFirstAudio() {
  int audio_id = AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiSupercellLogo);

  if (audio_id != AudioManager::kInvalidAudioId) {
    cocos2d::AudioEngine::setFinishCallback(
        audio_id, [this](int id, const std::string& file) {
          first_audio_done_ = true;
          loading_stage_ = 1;
          playSecondAudio();
        });
  } else {
    // 音效播放失败，直接标记完成
    CCLOG("⚠️ supercell_logo 音效播放失败");
    first_audio_done_ = true;
    loading_stage_ = 1;
    playSecondAudio();
  }
}

void LoadingLayer::playSecondAudio() {
  int audio_id = AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiLoadingEnter);

  if (audio_id != AudioManager::kInvalidAudioId) {
    cocos2d::AudioEngine::setFinishCallback(
        audio_id, [this](int id, const std::string& file) {
          second_audio_done_ = true;
          loading_stage_ = 2;
        });
  } else {
    // 音效播放失败，直接标记完成
    CCLOG("⚠️ loading_enter 音效播放失败");
    second_audio_done_ = true;
    loading_stage_ = 2;
  }
}

void LoadingLayer::finishLoading() {
  is_loading_ = false;

  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateProgress));
  this->unschedule(CC_SCHEDULE_SELECTOR(LoadingLayer::updateLoadingText));

  this->scheduleOnce(
      [this](float dt) {
        if (on_complete_callback_) {
          on_complete_callback_();
        }
      },
      kFinishDelay, "finish_loading");
}

void LoadingLayer::updateProgress(float dt) {
  if (!is_loading_) {
    return;
  }

  // 根据音效完成状态决定进度速度
  // 音效完成后快速推进，未完成时缓慢前进（避免卡顿）
  float speed = kBaseProgressSpeed;
  
  if (current_progress_ < 50.0f) {
    // 第一阶段：0-50%
    if (first_audio_done_) {
      speed = kFastProgressSpeed;
    }
  } else if (current_progress_ < 85.0f) {
    // 第二阶段：50-85%
    if (second_audio_done_) {
      speed = kFastProgressSpeed;
    }
  } else {
    // 第三阶段：85-100%
    if (second_audio_done_) {
      speed = kFastProgressSpeed;
    }
  }

  // 持续更新进度（不会在阶段切换处停止）
  current_progress_ = std::min(current_progress_ + speed * dt, 100.0f);

  // 更新进度条显示
  auto progress_fill = this->getChildByName<DrawNode*>("progress_fill");
  if (progress_fill) {
    progress_fill->clear();

    float fill_width = (bar_width_ - 4) * (current_progress_ / 100.0f);
    float fill_height = bar_height_ - 4;

    if (fill_width > 0) {
      // 渐变颜色：绿色 → 黄绿色
      float t = current_progress_ / 100.0f;
      Color4F fill_color(0.2f + 0.6f * t, 0.8f + 0.1f * t, 0.3f - 0.1f * t, 1.0f);

      progress_fill->drawSolidRect(
          Vec2(0, -fill_height / 2), Vec2(fill_width, fill_height / 2), fill_color);

      // 高光效果
      progress_fill->drawSolidRect(
          Vec2(0, fill_height / 4), Vec2(fill_width, fill_height / 2),
          Color4F(1.0f, 1.0f, 1.0f, 0.2f));
    }
  }

  // 进度完成且音效都已结束时触发完成
  if (current_progress_ >= 100.0f && second_audio_done_) {
    finishLoading();
  }
}

void LoadingLayer::updateLoadingText(float dt) {
  static int dot_count = 0;
  dot_count = (dot_count + 1) % 4;

  if (!loading_label_) {
    return;
  }

  std::string dots(dot_count, '.');
  std::string text;

  switch (loading_stage_) {
    case 0:
      text = "正在初始化" + dots;
      break;
    case 1:
      text = "正在加载资源" + dots;
      break;
    default:
      text = "即将进入游戏" + dots;
      break;
  }

  loading_label_->setString(text);
}
