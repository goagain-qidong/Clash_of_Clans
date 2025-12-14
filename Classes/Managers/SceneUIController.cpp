/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SceneUIController.cpp
 * File Function: 场景UI控制器 - 负责管理游戏场景中的UI元素
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "SceneUIController.h"
#include "../UI/SettingsPanel.h"

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
    
    return true;
}

void SceneUIController::setupMainButtons()
{
    float resourceXPos = 30;
    float buildButtonY = _visibleSize.height - 230;
    
    // Shop 按钮 (绿色)
    _shopButton = createFlatButton("Shop", Size(100, 50), Color3B(50, 150, 50), [this](Ref*) {
        if (_onShopClicked) _onShopClicked();
    });
    _shopButton->setPosition(Vec2(resourceXPos + 70, buildButtonY));
    this->addChild(_shopButton, 10);
    
    // Settings 按钮 (灰色)
    _settingsButton = createFlatButton("\xE2\x9A\x99", Size(60, 60), Color3B(100, 100, 100), [this](Ref*) {
        onSettingsClicked();
    });
    _settingsButton->setTitleFontSize(36);
    _settingsButton->setPosition(Vec2(_visibleSize.width - 60, _visibleSize.height - 160));
    this->addChild(_settingsButton, 10);
    
    // Attack 按钮 (橙色)
    _attackButton = createFlatButton("Attack!", Size(120, 60), Color3B(200, 80, 0), [this](Ref*) {
        if (_onAttackClicked) _onAttackClicked();
    });
    _attackButton->setPosition(Vec2(100, 100));
    this->addChild(_attackButton, 20);
    
    // Clan 按钮 (蓝色)
    _clanButton = createFlatButton("Clan", Size(100, 50), Color3B(50, 100, 150), [this](Ref*) {
        if (_onClanClicked) _onClanClicked();
    });
    _clanButton->setPosition(Vec2(_visibleSize.width - 80, 100));
    this->addChild(_clanButton, 20);
    
    // 🆕 Defense Log 按钮 (紫色) - 左下角
    _defenseLogButton = createFlatButton("Defense Log", Size(140, 50), Color3B(100, 50, 100), [this](Ref*) {
        if (_onDefenseLogClicked) _onDefenseLogClicked();
    });
    _defenseLogButton->setPosition(Vec2(100, 40)); // 左下角，稍微留点边距
    this->addChild(_defenseLogButton, 20);
}

cocos2d::ui::Button* SceneUIController::createFlatButton(const std::string& text, const cocos2d::Size& size, const cocos2d::Color3B& color, const std::function<void(cocos2d::Ref*)>& callback)
{
    auto button = Button::create();
    button->setTitleText(text);
    button->setTitleFontSize(20);
    button->setTitleColor(Color3B::WHITE);
    button->setContentSize(size);
    
    // 创建纯色背景
    auto bg = LayerColor::create(Color4B(color), size.width, size.height);
    bg->setPosition(Vec2::ZERO);
    // 确保背景在文字后面
    button->addChild(bg, -1);
    
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
        std::string sizeText = StringUtils::format("%dx%d", (int)building.gridSize.width, (int)building.gridSize.height);
        auto sizeLabel = Label::createWithSystemFont(sizeText, "Arial", 14);
        sizeLabel->setPosition(Vec2(120, 20));
        sizeLabel->setTextColor(Color4B::GREEN);
        item->addChild(sizeLabel);
        
        // 建筑花费
        std::string costText = StringUtils::format("Cost: %d", (int)building.cost);
        auto costLabel = Label::createWithSystemFont(costText, "Arial", 12);
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
    
    float buttonSize = 45.0f;
    float offsetX = 60.0f;
    float offsetY = 80.0f;
    
    // 确认按钮（绿色勾）
    _confirmButton = Button::create();
    _confirmButton->setTitleText("\xE2\x9C\x93");  // UTF-8编码的 ✓
    _confirmButton->setTitleFontSize(30);
    _confirmButton->setTitleColor(Color3B::WHITE);
    _confirmButton->setContentSize(Size(buttonSize, buttonSize));
    _confirmButton->setPosition(Vec2(worldPos.x + offsetX, worldPos.y + offsetY));
    
    auto confirmBg = LayerColor::create(Color4B(0, 200, 0, 200), buttonSize, buttonSize);
    confirmBg->setPosition(Vec2(-buttonSize / 2, -buttonSize / 2));
    _confirmButton->addChild(confirmBg, -1);
    
    _confirmButton->addClickEventListener([this](Ref*) {
        if (_onConfirmBuilding) _onConfirmBuilding();
    });
    this->addChild(_confirmButton, 10000);
    
    // 取消按钮（红色叉）
    _cancelButton = Button::create();
    _cancelButton->setTitleText("\xE2\x9C\x97");  // UTF-8编码的 ✗
    _cancelButton->setTitleFontSize(30);
    _cancelButton->setTitleColor(Color3B::WHITE);
    _cancelButton->setContentSize(Size(buttonSize, buttonSize));
    _cancelButton->setPosition(Vec2(worldPos.x - offsetX, worldPos.y + offsetY));
    
    auto cancelBg = LayerColor::create(Color4B(200, 0, 0, 200), buttonSize, buttonSize);
    cancelBg->setPosition(Vec2(-buttonSize / 2, -buttonSize / 2));
    _cancelButton->addChild(cancelBg, -1);
    
    _cancelButton->addClickEventListener([this](Ref*) {
        if (_onCancelBuilding) _onCancelBuilding();
    });
    this->addChild(_cancelButton, 10000);
    
    // 弹出动画
    auto scaleIn = ScaleTo::create(0.2f, 1.0f);
    _confirmButton->setScale(0.0f);
    _confirmButton->runAction(EaseBackOut::create(scaleIn->clone()));
    _cancelButton->setScale(0.0f);
    _cancelButton->runAction(EaseBackOut::create(scaleIn->clone()));
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
    if (_shopButton) _shopButton->setVisible(visible);
}

void SceneUIController::setAttackButtonVisible(bool visible)
{
    if (_attackButton) _attackButton->setVisible(visible);
}

void SceneUIController::setClanButtonVisible(bool visible)
{
    if (_clanButton) _clanButton->setVisible(visible);
}
