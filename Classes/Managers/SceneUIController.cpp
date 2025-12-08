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
    
    // Shop 按钮
    _shopButton = Button::create();
    _shopButton->setTitleText("Shop");
    _shopButton->setTitleFontSize(24);
    _shopButton->setContentSize(Size(100, 50));
    _shopButton->setPosition(Vec2(resourceXPos + 70, buildButtonY));
    _shopButton->addClickEventListener([this](Ref*) {
        if (_onShopClicked) _onShopClicked();
    });
    this->addChild(_shopButton, 10);
    
    // Settings 按钮 (齿轮图标)
    _settingsButton = Button::create();
    _settingsButton->setTitleText("\xE2\x9A\x99");  // UTF-8编码的 ⚙
    _settingsButton->setTitleFontSize(32);
    _settingsButton->setContentSize(Size(60, 60));
    _settingsButton->setPosition(Vec2(_visibleSize.width - 60, _visibleSize.height - 160));
    _settingsButton->addClickEventListener([this](Ref*) {
        onSettingsClicked();
    });
    this->addChild(_settingsButton, 10);
    
    // Attack 按钮
    _attackButton = Button::create();
    _attackButton->setTitleText("Attack!");
    _attackButton->setTitleFontSize(24);
    _attackButton->setPosition(Vec2(100, 100));
    _attackButton->addClickEventListener([this](Ref*) {
        if (_onAttackClicked) _onAttackClicked();
    });
    this->addChild(_attackButton, 20);
    
    // Clan 按钮
    _clanButton = Button::create();
    _clanButton->setTitleText("Clan");
    _clanButton->setTitleFontSize(24);
    _clanButton->setPosition(Vec2(_visibleSize.width - 50, 100));
    _clanButton->addClickEventListener([this](Ref*) {
        if (_onClanClicked) _onClanClicked();
    });
    this->addChild(_clanButton, 20);
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
