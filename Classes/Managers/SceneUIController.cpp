/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SceneUIController.cpp
 * File Function: 场景UI控制器 - 负责管理游戏场景中的UI元素
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "SceneUIController.h"
#include "../Managers/AccountManager.h" // Ensure AccountManager is included
#include "../Managers/SocketClient.h"
#include "../Scenes/BattleScene.h"
#include "../UI/SettingsPanel.h"
#include "json/document.h"

USING_NS_CC;
using namespace ui;

bool SceneUIController::init()
{
    if (!Node::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    setupMainButtons();

    // 🔴 修复：移除全局PVP回调设置
    // 这个回调会与ClanPanel的回调冲突，导致点击"部落"按钮时
    // 错误地触发异步对战的玩家选择界面
    // PVP回调应该只在ClanPanel中设置和管理

    // 原代码已删除:
    // SocketClient::getInstance().setOnPvpStart([](const std::string& role, ...){...});

    return true;
}

void SceneUIController::setupMainButtons()
{
    float margin = 20.0f; // 边距

    // ==================== 右下角按钮组 ====================
    // Shop 按钮 - 右下角
    _shopButton = Button::create("icon/shop_icon.png");
    if (_shopButton->getContentSize().equals(Size::ZERO))
    {
        _shopButton = createFlatButton("Shop", Size(120, 120), Color3B(50, 150, 50), [this](Ref*) {
            if (_onShopClicked)
                _onShopClicked();
        });
    }
    else
    {
        _shopButton->setScale(90.0f / _shopButton->getContentSize().width);
        _shopButton->addClickEventListener([this](Ref*) {
            if (_onShopClicked)
                _onShopClicked();
        });
    }
    _shopButton->setPosition(Vec2(_visibleSize.width - 80, 80)); // 右下角
    this->addChild(_shopButton, 10);

    // Settings 按钮 - 右下角（Shop 按钮上方）
    _settingsButton = Button::create("icon/tool_icon.png");
    if (_settingsButton->getContentSize().equals(Size::ZERO))
    {
        _settingsButton = createFlatButton("\xE2\x9A\x99", Size(60, 60), Color3B(100, 100, 100),
                                           [this](Ref*) { onSettingsClicked(); });
        _settingsButton->setTitleFontSize(36);
    }
    else
    {
        _settingsButton->setScale(60.0f / _settingsButton->getContentSize().width);
        _settingsButton->addClickEventListener([this](Ref*) { onSettingsClicked(); });
    }
    _settingsButton->setPosition(Vec2(_visibleSize.width - 60, 160)); // 右下角，Shop 上方
    this->addChild(_settingsButton, 10);

    // ==================== 左下角按钮组 ====================
    // Attack 按钮 - 左下角
    _attackButton = Button::create("icon/attack_icon.png");
    if (_attackButton->getContentSize().equals(Size::ZERO))
    {
        _attackButton = createFlatButton("Attack!", Size(120, 60), Color3B(200, 80, 0), [this](Ref*) {
            if (_onAttackClicked)
                _onAttackClicked();
        });
    }
    else
    {
        _attackButton->setScale(110.0f / _attackButton->getContentSize().width);
        _attackButton->addClickEventListener([this](Ref*) {
            if (_onAttackClicked)
                _onAttackClicked();
        });
    }
    _attackButton->setPosition(Vec2(70, 90));
    this->addChild(_attackButton, 20);

    // ==================== 左侧中间按钮 ====================
    // Clan 按钮 - 左侧中间
    _clanButton = Button::create("icon/clan_icon.png");
    if (_clanButton->getContentSize().equals(Size::ZERO))
    {
        _clanButton = createFlatButton("Clan", Size(100, 50), Color3B(50, 100, 150), [this](Ref*) {
            if (_onClanClicked)
                _onClanClicked();
        });
    }
    else
    {
        _clanButton->setScale(90.0f / _clanButton->getContentSize().width);
        _clanButton->addClickEventListener([this](Ref*) {
            if (_onClanClicked)
                _onClanClicked();
        });
    }
    _clanButton->setPosition(Vec2(60, _visibleSize.height / 2)); // 左侧中间
    this->addChild(_clanButton, 20);

    // Defense Log 按钮 - 左下角（Attack 按钮上方）
    _defenseLogButton = Button::create("icon/defense_log_icon.png");
    if (_defenseLogButton->getContentSize().equals(Size::ZERO))
    {
        _defenseLogButton = createFlatButton("Defense Log", Size(140, 50), Color3B(100, 50, 100), [this](Ref*) {
            if (_onDefenseLogClicked)
                _onDefenseLogClicked();
        });
    }
    else
    {
        _defenseLogButton->setScale(110.0f / _defenseLogButton->getContentSize().width);
        _defenseLogButton->addClickEventListener([this](Ref*) {
            if (_onDefenseLogClicked)
                _onDefenseLogClicked();
        });
    }
    _defenseLogButton->setPosition(Vec2(60, 180));
    this->addChild(_defenseLogButton, 20);
}

cocos2d::ui::Button* SceneUIController::createFlatButton(const std::string& text, const cocos2d::Size& size,
                                                         const cocos2d::Color3B&                   color,
                                                         const std::function<void(cocos2d::Ref*)>& callback)
{
    auto button = Button::create();
    // 强制使用自定义大小，确保布局正确
    button->ignoreContentAdaptWithSize(false);
    button->setContentSize(size);

    button->setTitleText(text);
    button->setTitleFontSize(20);
    button->setTitleColor(Color3B::WHITE);
    // 确保文字居中
    if (button->getTitleRenderer())
    {
        button->getTitleRenderer()->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
        // 调整Label位置到中心
        button->getTitleRenderer()->setPosition(Vec2(size.width / 2, size.height / 2));
    }

    button->addClickEventListener(callback);

    // 添加点击缩放效果
    button->setPressedActionEnabled(true);
    button->setZoomScale(0.1f);

    return button;
}

void SceneUIController::onSettingsClicked()
{
    auto settingsPanel = SettingsPanel::create();
    this->getParent()->addChild(settingsPanel, 10000);

    // 设置回调
    settingsPanel->setOnAccountSwitched([this]() {
        if (_onAccountSwitched)
        {
            _onAccountSwitched();
        }
    });

    settingsPanel->setOnLogout([this]() {
        if (_onLogout)
        {
            _onLogout();
        }
    });

    settingsPanel->setOnMapChanged([this](const std::string& newMap) {
        if (_onMapChanged)
        {
            _onMapChanged(newMap);
        }
    });

    settingsPanel->show();
}

void SceneUIController::setBuildingList(const std::vector<BuildingData>& buildings)
{
    _buildingList = buildings;
    createBuildingListUI();
}

void SceneUIController::createBuildingListUI()
{
    if (_buildingListUI)
    {
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

    for (const auto& building : _buildingList)
    {
        auto item = Layout::create();
        item->setContentSize(Size(280, 60));
        item->setTouchEnabled(true);

        // 建筑图标
        auto sprite = Sprite::create(building.imageFile);
        if (sprite)
        {
            sprite->setScale(0.3f);
            sprite->setPosition(Vec2(40, 30));
            item->addChild(sprite);
        }

        // 建筑名称
        auto nameLabel = Label::createWithSystemFont(building.name, "Arial", 16);
        nameLabel->setPosition(Vec2(120, 40));
        nameLabel->setTextColor(Color4B::YELLOW);
        item->addChild(nameLabel);

        // 建筑大小
        std::string sizeText =
            StringUtils::format("%dx%d", (int)building.gridSize.width, (int)building.gridSize.height);
        auto sizeLabel = Label::createWithSystemFont(sizeText, "Arial", 14);
        sizeLabel->setPosition(Vec2(120, 20));
        sizeLabel->setTextColor(Color4B::GREEN);
        item->addChild(sizeLabel);

        // 建筑花费
        std::string costText  = StringUtils::format("Cost: %d", (int)building.cost);
        auto        costLabel = Label::createWithSystemFont(costText, "Arial", 12);
        costLabel->setPosition(Vec2(220, 40));
        costLabel->setTextColor(Color4B::WHITE);
        item->addChild(costLabel);

        // 背景
        auto bg = LayerColor::create(Color4B(40, 40, 60, 255));
        bg->setContentSize(Size(280, 60));
        item->addChild(bg, -1);

        // 点击事件
        item->addClickEventListener([this, building](Ref*) {
            if (_onBuildingSelected)
            {
                _onBuildingSelected(building);
            }
            toggleBuildingList();
        });

        _buildingListUI->pushBackCustomItem(item);
    }

    this->addChild(_buildingListUI, 20);
}

void SceneUIController::toggleBuildingList()
{
    if (!_buildingListUI)
        return;

    _isBuildingListVisible = !_isBuildingListVisible;
    _buildingListUI->setVisible(_isBuildingListVisible);
}

void SceneUIController::showConfirmButtons(const Vec2& worldPos)
{
    hideConfirmButtons();

    float buttonSize = 36.0f;
    float offsetX    = 50.0f;
    float offsetY    = 70.0f;

    // 确认按钮（绿色勾 - 使用confirm_button.png）
    _confirmButton = Button::create("icon/confirm_button.png");
    float confirmScale =
        buttonSize / std::max(_confirmButton->getContentSize().width, _confirmButton->getContentSize().height);
    if (_confirmButton->getContentSize().equals(Size::ZERO))
    {
        // 如果图片不存在，回退到文本模式
        _confirmButton = Button::create();
        _confirmButton->setTitleText("\xE2\x9C\x93"); // UTF-8编码的 ✓
        _confirmButton->setTitleFontSize(24);
        _confirmButton->setTitleColor(Color3B::WHITE);
        _confirmButton->setContentSize(Size(buttonSize, buttonSize));
        confirmScale = 1.0f;
    }
    _confirmButton->setPosition(Vec2(worldPos.x + offsetX, worldPos.y + offsetY));

    _confirmButton->addClickEventListener([this](Ref*) {
        if (_onConfirmBuilding)
            _onConfirmBuilding();
    });
    this->addChild(_confirmButton, 10000);

    // 取消按钮（红色叉 - 使用return_button.png）
    _cancelButton = Button::create("icon/return_button.png");
    float cancelScale =
        buttonSize / std::max(_cancelButton->getContentSize().width, _cancelButton->getContentSize().height);
    if (_cancelButton->getContentSize().equals(Size::ZERO))
    {
        // 如果图片不存在，回退到文本模式
        _cancelButton = Button::create();
        _cancelButton->setTitleText("\xE2\x9C\x97"); // UTF-8编码的 ✗
        _cancelButton->setTitleFontSize(24);
        _cancelButton->setTitleColor(Color3B::WHITE);
        _cancelButton->setContentSize(Size(buttonSize, buttonSize));
        cancelScale = 1.0f;
    }
    _cancelButton->setPosition(Vec2(worldPos.x - offsetX, worldPos.y + offsetY));

    _cancelButton->addClickEventListener([this](Ref*) {
        if (_onCancelBuilding)
            _onCancelBuilding();
    });
    this->addChild(_cancelButton, 10000);

    _confirmButton->setScale(0.0f);
    _confirmButton->runAction(EaseBackOut::create(ScaleTo::create(0.2f, confirmScale)));

    _cancelButton->setScale(0.0f);
    _cancelButton->runAction(EaseBackOut::create(ScaleTo::create(0.2f, cancelScale)));
}

void SceneUIController::hideConfirmButtons()
{
    if (_confirmButton)
    {
        _confirmButton->removeFromParent();
        _confirmButton = nullptr;
    }

    if (_cancelButton)
    {
        _cancelButton->removeFromParent();
        _cancelButton = nullptr;
    }
}

void SceneUIController::showHint(const std::string& hint)
{
    if (_hintLabel)
    {
        _hintLabel->removeFromParent();
    }

    _hintLabel = Label::createWithSystemFont(hint, "Arial", 18);
    _hintLabel->setPosition(Vec2(_visibleSize.width / 2, 100));
    _hintLabel->setTextColor(Color4B::YELLOW);
    this->addChild(_hintLabel, 30);
}

void SceneUIController::hideHint()
{
    if (_hintLabel)
    {
        _hintLabel->removeFromParent();
        _hintLabel = nullptr;
    }
}

void SceneUIController::setShopButtonVisible(bool visible)
{
    if (_shopButton)
        _shopButton->setVisible(visible);
}

void SceneUIController::setAttackButtonVisible(bool visible)
{
    if (_attackButton)
        _attackButton->setVisible(visible);
}

void SceneUIController::setClanButtonVisible(bool visible)
{
    if (_clanButton)
        _clanButton->setVisible(visible);
}