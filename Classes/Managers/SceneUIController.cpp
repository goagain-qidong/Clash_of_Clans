/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SceneUIController.cpp
 * File Function: 场景UI控制器 - 负责管理游戏场景中的UI元素
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "SceneUIController.h"
#include "../Managers/AccountManager.h"
#include "../Managers/SocketClient.h"
#include "../Scenes/BattleScene.h"
#include "../UI/SettingsPanel.h"
#include "Audio/AudioManager.h"
#include "json/document.h"

USING_NS_CC;
using namespace ui;

namespace {
// UI布局常量
constexpr float kMargin = 20.0f;
constexpr float kShopButtonSize = 90.0f;
constexpr float kSettingsButtonSize = 60.0f;
constexpr float kAttackButtonWidth = 110.0f;
constexpr float kClanButtonSize = 90.0f;
constexpr float kDefenseLogButtonWidth = 110.0f;
constexpr float kConfirmButtonSize = 48.0f;
constexpr float kExitButtonWidth = 120.0f;
constexpr float kExitButtonHeight = 44.0f;
constexpr float kExitButtonBottomMargin = 140.0f;

// 辅助函数：创建带图标的按钮，图标加载失败时回退到文字按钮
Button* CreateIconButton(const std::string& icon_path,
                         const std::string& fallback_text,
                         float target_size,
                         const Color3B& fallback_color,
                         const std::function<void(Ref*)>& callback) {
  auto button = Button::create(icon_path);
  if (button->getContentSize().equals(Size::ZERO)) {
    // 图标加载失败，创建文字按钮
    button = Button::create();
    button->ignoreContentAdaptWithSize(false);
    button->setContentSize(Size(target_size, target_size));
    button->setTitleText(fallback_text);
    button->setTitleFontSize(20);
    button->setTitleColor(Color3B::WHITE);
    button->setPressedActionEnabled(true);
    button->setZoomScale(0.1f);
  } else {
    button->setScale(target_size / button->getContentSize().width);
  }
  button->addClickEventListener([callback](Ref* sender) {
    AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
    if (callback) callback(sender);
  });
  return button;
}
}  // namespace

bool SceneUIController::init() {
  if (!Node::init()) {
    return false;
  }

  _visibleSize = Director::getInstance()->getVisibleSize();
  setupMainButtons();
  return true;
}

void SceneUIController::setupMainButtons() {
  // ==================== 右下角按钮组 ====================
  // 商店按钮
  _shopButton = CreateIconButton(
      "icon/shop_icon.png", "Shop", kShopButtonSize, Color3B(50, 150, 50),
      [this](Ref*) { if (_onShopClicked) _onShopClicked(); });
  _shopButton->setPosition(Vec2(_visibleSize.width - 80, 80));
  this->addChild(_shopButton, 10);

  // 设置按钮
  _settingsButton = CreateIconButton(
      "icon/tool_icon.png", "\xE2\x9A\x99", kSettingsButtonSize, Color3B(100, 100, 100),
      [this](Ref*) { onSettingsClicked(); });
  if (_settingsButton->getTitleLabel()) {
    _settingsButton->setTitleFontSize(36);
  }
  _settingsButton->setPosition(Vec2(_visibleSize.width - 60, 160));
  this->addChild(_settingsButton, 10);

  // ==================== 左下角按钮组 ====================
  // 攻击按钮
  _attackButton = CreateIconButton(
      "icon/attack_icon.png", "Attack!", kAttackButtonWidth, Color3B(200, 80, 0),
      [this](Ref*) { if (_onAttackClicked) _onAttackClicked(); });
  _attackButton->setPosition(Vec2(70, 90));
  this->addChild(_attackButton, 20);

  // 防御日志按钮
  _defenseLogButton = CreateIconButton(
      "icon/defense_log_icon.png", "Defense Log", kDefenseLogButtonWidth, Color3B(100, 50, 100),
      [this](Ref*) { if (_onDefenseLogClicked) _onDefenseLogClicked(); });
  _defenseLogButton->setPosition(Vec2(60, 180));
  this->addChild(_defenseLogButton, 20);

  // ==================== 左侧中间按钮 ====================
  // 部落按钮
  _clanButton = CreateIconButton(
      "icon/clan_icon.png", "Clan", kClanButtonSize, Color3B(50, 100, 150),
      [this](Ref*) { if (_onClanClicked) _onClanClicked(); });
  _clanButton->setPosition(Vec2(60, _visibleSize.height / 2));
  this->addChild(_clanButton, 20);
}

cocos2d::ui::Button* SceneUIController::createFlatButton(
    const std::string& text, const cocos2d::Size& size,
    const cocos2d::Color3B& color,
    const std::function<void(cocos2d::Ref*)>& callback) {
  auto button = Button::create();
  button->ignoreContentAdaptWithSize(false);
  button->setContentSize(size);
  button->setTitleText(text);
  button->setTitleFontSize(20);
  button->setTitleColor(Color3B::WHITE);

  if (button->getTitleRenderer()) {
    button->getTitleRenderer()->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
    button->getTitleRenderer()->setPosition(Vec2(size.width / 2, size.height / 2));
  }

  button->addClickEventListener(callback);
  button->setPressedActionEnabled(true);
  button->setZoomScale(0.1f);
  return button;
}

void SceneUIController::onSettingsClicked() {
  auto settings_panel = SettingsPanel::create();
  this->getParent()->addChild(settings_panel, 10000);

  settings_panel->setOnAccountSwitched([this]() {
    if (_onAccountSwitched) _onAccountSwitched();
  });

  settings_panel->setOnLogout([this]() {
    if (_onLogout) _onLogout();
  });

  settings_panel->setOnMapChanged([this](const std::string& new_map) {
    if (_onMapChanged) _onMapChanged(new_map);
  });

  settings_panel->show();
}

void SceneUIController::setBuildingList(const std::vector<BuildingData>& buildings) {
  _buildingList = buildings;
  createBuildingListUI();
}

void SceneUIController::createBuildingListUI() {
  if (_buildingListUI) {
    _buildingListUI->removeFromParent();
  }

  _buildingListUI = ListView::create();
  _buildingListUI->setContentSize(Size(300, 200));
  _buildingListUI->setPosition(Vec2(160, _visibleSize.height - 250));
  _buildingListUI->setBackGroundColor(Color3B(60, 60, 80));
  _buildingListUI->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
  _buildingListUI->setOpacity(220);
  _buildingListUI->setVisible(false);
  _buildingListUI->setScrollBarEnabled(true);
  _buildingListUI->setBounceEnabled(true);

  for (const auto& building : _buildingList) {
    auto item = Layout::create();
    item->setContentSize(Size(280, 60));
    item->setTouchEnabled(true);

    // 建筑图标
    auto sprite = Sprite::create(building.imageFile);
    if (sprite) {
      sprite->setScale(0.3f);
      sprite->setPosition(Vec2(40, 30));
      item->addChild(sprite);
    }

    // 建筑名称
    auto name_label = Label::createWithSystemFont(building.name, "Arial", 16);
    name_label->setPosition(Vec2(120, 40));
    name_label->setTextColor(Color4B::YELLOW);
    item->addChild(name_label);

    // 建筑尺寸
    auto size_label = Label::createWithSystemFont(
        StringUtils::format("%dx%d", static_cast<int>(building.gridSize.width),
                            static_cast<int>(building.gridSize.height)),
        "Arial", 14);
    size_label->setPosition(Vec2(120, 20));
    size_label->setTextColor(Color4B::GREEN);
    item->addChild(size_label);

    // 建筑花费
    auto cost_label = Label::createWithSystemFont(
        StringUtils::format("Cost: %d", static_cast<int>(building.cost)), "Arial", 12);
    cost_label->setPosition(Vec2(220, 40));
    cost_label->setTextColor(Color4B::WHITE);
    item->addChild(cost_label);

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 255));
    bg->setContentSize(Size(280, 60));
    item->addChild(bg, -1);

    item->addClickEventListener([this, building](Ref*) {
      if (_onBuildingSelected) {
        _onBuildingSelected(building);
      }
      toggleBuildingList();
    });

    _buildingListUI->pushBackCustomItem(item);
  }

  this->addChild(_buildingListUI, 20);
}

void SceneUIController::toggleBuildingList() {
  if (!_buildingListUI) return;

  _isBuildingListVisible = !_isBuildingListVisible;
  _buildingListUI->setVisible(_isBuildingListVisible);
}

void SceneUIController::showConfirmButtons(const Vec2& worldPos) {
  hideConfirmButtons();

  constexpr float kOffsetX = 60.0f;
  constexpr float kOffsetY = 80.0f;

  // 确认按钮
  _confirmButton = Button::create("icon/confirm_button.png");
  float confirm_scale = kConfirmButtonSize /
      std::max(_confirmButton->getContentSize().width, _confirmButton->getContentSize().height);
  if (_confirmButton->getContentSize().equals(Size::ZERO)) {
    _confirmButton = Button::create();
    _confirmButton->setTitleText("\xE2\x9C\x93");
    _confirmButton->setTitleFontSize(28);
    _confirmButton->setTitleColor(Color3B::WHITE);
    _confirmButton->setContentSize(Size(kConfirmButtonSize, kConfirmButtonSize));
    confirm_scale = 1.0f;
  }
  _confirmButton->setPosition(Vec2(worldPos.x + kOffsetX, worldPos.y + kOffsetY));
  _confirmButton->addClickEventListener([this](Ref*) {
    AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
    if (_onConfirmBuilding) _onConfirmBuilding();
  });
  this->addChild(_confirmButton, 10000);

  // 取消按钮
  _cancelButton = Button::create("icon/return_button.png");
  float cancel_scale = kConfirmButtonSize /
      std::max(_cancelButton->getContentSize().width, _cancelButton->getContentSize().height);
  if (_cancelButton->getContentSize().equals(Size::ZERO)) {
    _cancelButton = Button::create();
    _cancelButton->setTitleText("\xE2\x9C\x97");
    _cancelButton->setTitleFontSize(28);
    _cancelButton->setTitleColor(Color3B::WHITE);
    _cancelButton->setContentSize(Size(kConfirmButtonSize, kConfirmButtonSize));
    cancel_scale = 1.0f;
  }
  _cancelButton->setPosition(Vec2(worldPos.x - kOffsetX, worldPos.y + kOffsetY));
  _cancelButton->addClickEventListener([this](Ref*) {
    AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
    if (_onCancelBuilding) _onCancelBuilding();
  });
  this->addChild(_cancelButton, 10000);

  // 入场动画
  _confirmButton->setScale(0.0f);
  _confirmButton->runAction(EaseBackOut::create(ScaleTo::create(0.2f, confirm_scale)));

  _cancelButton->setScale(0.0f);
  _cancelButton->runAction(EaseBackOut::create(ScaleTo::create(0.2f, cancel_scale)));
}

void SceneUIController::hideConfirmButtons() {
  if (_confirmButton) {
    _confirmButton->removeFromParent();
    _confirmButton = nullptr;
  }
  if (_cancelButton) {
    _cancelButton->removeFromParent();
    _cancelButton = nullptr;
  }
}

void SceneUIController::showExitBuildModeButton() {
  hideExitBuildModeButton();

  // 按钮尺寸调整以适应更长的文字
  constexpr float kButtonWidth = 180.0f;

  // 创建纯文字按钮
  _exitBuildModeButton = Button::create();
  _exitBuildModeButton->ignoreContentAdaptWithSize(false);
  _exitBuildModeButton->setContentSize(Size(kButtonWidth, kExitButtonHeight));
  _exitBuildModeButton->setAnchorPoint(Vec2(0.5f, 0.5f));

  _exitBuildModeButton->setTitleText("退出放置并收回建筑");
  _exitBuildModeButton->setTitleFontSize(18);
  _exitBuildModeButton->setTitleColor(Color3B::WHITE);

  // 红色背景
  auto bg = LayerColor::create(Color4B(200, 60, 60, 230), kButtonWidth, kExitButtonHeight);
  bg->setAnchorPoint(Vec2::ZERO);
  bg->setPosition(Vec2::ZERO);
  _exitBuildModeButton->addChild(bg, -1);

  if (_exitBuildModeButton->getTitleRenderer()) {
    _exitBuildModeButton->getTitleRenderer()->setPosition(
        Vec2(kButtonWidth / 2, kExitButtonHeight / 2));
  }

  _exitBuildModeButton->setPosition(Vec2(_visibleSize.width / 2, kExitButtonBottomMargin));
  _exitBuildModeButton->setTouchEnabled(true);
  _exitBuildModeButton->setPressedActionEnabled(true);
  _exitBuildModeButton->setZoomScale(0.1f);

  _exitBuildModeButton->addClickEventListener([this](Ref*) {
    AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
    if (_onExitBuildMode) _onExitBuildMode();
  });

  this->addChild(_exitBuildModeButton, 10001);

  // 入场动画
  Vec2 target_pos = _exitBuildModeButton->getPosition();
  _exitBuildModeButton->setPosition(Vec2(target_pos.x, target_pos.y - 60));
  _exitBuildModeButton->setOpacity(0);

  _exitBuildModeButton->runAction(Spawn::create(
      EaseBackOut::create(MoveTo::create(0.25f, target_pos)),
      FadeIn::create(0.2f), nullptr));

  CCLOG("显示退出放置按钮");
}

void SceneUIController::hideExitBuildModeButton() {
  if (!_exitBuildModeButton) return;

  Vec2 current_pos = _exitBuildModeButton->getPosition();
  _exitBuildModeButton->runAction(Sequence::create(
      Spawn::create(
          EaseBackIn::create(MoveTo::create(0.2f, Vec2(current_pos.x, current_pos.y - 60))),
          FadeOut::create(0.15f), nullptr),
      RemoveSelf::create(), nullptr));

  _exitBuildModeButton = nullptr;
  CCLOG("隐藏退出放置按钮");
}

void SceneUIController::showHint(const std::string& hint) {
  if (_hintLabel) {
    _hintLabel->removeFromParent();
  }

  _hintLabel = Label::createWithSystemFont(hint, "Arial", 18);
  _hintLabel->setPosition(Vec2(_visibleSize.width / 2, 100));
  _hintLabel->setTextColor(Color4B::YELLOW);
  this->addChild(_hintLabel, 30);
}

void SceneUIController::hideHint() {
  if (_hintLabel) {
    _hintLabel->removeFromParent();
    _hintLabel = nullptr;
  }
}

void SceneUIController::setShopButtonVisible(bool visible) {
  if (_shopButton) _shopButton->setVisible(visible);
}

void SceneUIController::setAttackButtonVisible(bool visible) {
  if (_attackButton) _attackButton->setVisible(visible);
}

void SceneUIController::setClanButtonVisible(bool visible) {
  if (_clanButton) _clanButton->setVisible(visible);
}