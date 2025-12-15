/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeUI.cpp
 * File Function: 通用建筑升级界面实现
 * Author:
 * Update Date:
 * License:       MIT License
 ****************************************************************/

#include "BuildingUpgradeUI.h"
#include "ArmyBuilding.h"
#include "Managers/UpgradeManager.h"
#include "ResourceManager.h"
#include "SceneUIController.h"
#include "Services/BuildingUpgradeService.h"
#include "Unit/TrainingUI.h"

USING_NS_CC;
using namespace ui;

BuildingUpgradeUI* BuildingUpgradeUI::create(BaseBuilding* building)
{
    BuildingUpgradeUI* ui = new (std::nothrow) BuildingUpgradeUI();
    if (ui && ui->init(building))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool BuildingUpgradeUI::init(BaseBuilding* building)
{
    if (!Node::init())
    {
        return false;
    }

    _building = building;
    setupUI();
    updateUI();
    return true;
}

void BuildingUpgradeUI::setupUI()
{
    // ============================================================
    // 1. 面板基础设置
    // ============================================================
    bool  isBarracks  = (_building && _building->getBuildingType() == BuildingType::kArmy);
    float panelHeight = isBarracks ? 340.0f : 260.0f;

    _panel = Layout::create();
    _panel->setContentSize(Size(280, panelHeight));
    _panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _panel->setBackGroundColor(Color3B(40, 40, 60));
    _panel->setBackGroundColorOpacity(230);
    _panel->setAnchorPoint(Vec2(0.5f, 0.0f));
    _panel->setTouchEnabled(true); // 面板阻挡穿透
    this->addChild(_panel);

    float panelWidth = _panel->getContentSize().width;
    float centerX    = panelWidth / 2.0f;

    // ============================================================
    // 2. 文字信息
    // ============================================================

    _titleLabel = Label::createWithSystemFont("", "Microsoft YaHei", 20);
    _titleLabel->setPosition(Vec2(centerX, panelHeight - 25));
    _titleLabel->setTextColor(Color4B::YELLOW);
    _panel->addChild(_titleLabel);

    _levelLabel = Label::createWithSystemFont("", "Microsoft YaHei", 16);
    _levelLabel->setPosition(Vec2(centerX, panelHeight - 50));
    _levelLabel->setTextColor(Color4B::WHITE);
    _panel->addChild(_levelLabel);

    _descLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _descLabel->setPosition(Vec2(centerX, panelHeight - 85));
    _descLabel->setTextColor(Color4B(200, 200, 200, 255));
    _descLabel->setAlignment(TextHAlignment::CENTER);
    _panel->addChild(_descLabel);

    _costLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _costLabel->setPosition(Vec2(centerX, panelHeight - 120));
    _costLabel->setTextColor(Color4B::GREEN);
    _panel->addChild(_costLabel);

    _timeLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _timeLabel->setPosition(Vec2(centerX, panelHeight - 145));
    _timeLabel->setTextColor(Color4B(150, 200, 255, 255));
    _panel->addChild(_timeLabel);

    auto gemTip = Label::createWithSystemFont("使用50宝石可将升级时间设置为10秒", "Microsoft YaHei", 12);
    gemTip->setPosition(Vec2(centerX, panelHeight - 170));
    gemTip->setTextColor(Color4B(240, 220, 100, 255));
    _panel->addChild(gemTip);

    // ============================================================
    // 3. 底部三个按钮 (紧贴对齐，无缝隙)
    // ============================================================

    // 按钮参数
    float btnWidth  = 90.0f; // 按钮宽度
    float btnHeight = 40.0f;
    float bottomY   = 40.0f; // 按钮中心距底部高度

    // 计算起始X (左边按钮的中心点)
    // 3个按钮总宽 = btnWidth * 3
    // StartX = (Panel宽 - 总宽)/2 + 半个按钮宽
    float startX = (panelWidth - (btnWidth * 3)) / 2 + btnWidth / 2;

    // --- A. 升级按钮 (绿色) ---
    _upgradeButton = Button::create();
    _upgradeButton->setContentSize(Size(btnWidth, btnHeight));
    _upgradeButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    _upgradeButton->setPosition(Vec2(startX, bottomY)); // 位置1
    _upgradeButton->setTitleText("升级");
    _upgradeButton->setTitleFontSize(16);
    _upgradeButton->setTitleColor(Color3B::WHITE);
    _upgradeButton->setTouchEnabled(true);
    _upgradeButton->setPressedActionEnabled(true);
    _upgradeButton->addClickEventListener([this](Ref*) { onUpgradeClicked(); });

    // auto upgradeBg = LayerColor::create(Color4B(0, 150, 0, 200), btnWidth, btnHeight);
    // upgradeBg->setAnchorPoint(Vec2::ZERO);
    // upgradeBg->setPosition(Vec2::ZERO);
    // // upgradeBg->setTouchEnabled(false); // 关键：背景不吃点击
    // _upgradeButton->addChild(upgradeBg, -1);
    _panel->addChild(_upgradeButton);

    // --- B. 宝石按钮 (紫色) ---
    auto gemButton = Button::create();
    gemButton->setContentSize(Size(btnWidth, btnHeight));
    gemButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    gemButton->setPosition(Vec2(startX + btnWidth, bottomY)); // 位置2：紧挨着
    // 不用图片，直接设置文字
    gemButton->setTitleText("宝石50"); // 尝试用emoji，如果不显示会自动变成方块，也可以改成 "宝石 50"
    gemButton->setTitleFontSize(16);
    gemButton->setTitleColor(Color3B::YELLOW);
    gemButton->setTouchEnabled(true);         // 关键：开启点击
    gemButton->setPressedActionEnabled(true); // 开启缩放反馈
    gemButton->setName("gemButton");

    // auto gemBg = LayerColor::create(Color4B(80, 0, 120, 200), btnWidth, btnHeight); // 紫色背景
    // gemBg->setAnchorPoint(Vec2::ZERO);
    // gemBg->setPosition(Vec2::ZERO);
    // // gemBg->setTouchEnabled(false); // 关键：背景不吃点击
    // gemButton->addChild(gemBg, -1);

    // 重新写一遍点击逻辑，确保无误
    gemButton->addClickEventListener([this](Ref* sender) {
        CCLOG("Gem Button Clicked!");
        if (!_building)
            return;

        auto& resMgr     = ResourceManager::getInstance();
        auto* upgradeMgr = UpgradeManager::getInstance();

        if (_building->isMaxLevel())
            return;
        // 检查工人
        if (!upgradeMgr->canStartUpgrade(_building, true))
        {
            CCLOG("No builder available");
            return;
        }

        int          cost     = _building->getUpgradeCost();
        ResourceType costType = _building->getUpgradeCostType();
        const int    GEM_COST = 50;

        // 检查资源
        bool hasRes = resMgr.hasEnough(costType, cost);
        bool hasGem = resMgr.hasEnough(ResourceType::kGem, GEM_COST);

        if (hasRes && hasGem)
        {
            // 扣除
            resMgr.consume(ResourceType::kGem, GEM_COST);
            if (resMgr.consume(costType, cost))
            {
                // 开始升级 (10秒)
                if (upgradeMgr->startUpgrade(_building, cost, 10.0f, true))
                {
                    if (_resultCallback)
                        _resultCallback(true, _building->getLevel());
                    onCloseClicked();
                }
                else
                {
                    // 失败回退
                    resMgr.addResource(costType, cost);
                    resMgr.addResource(ResourceType::kGem, GEM_COST);
                }
            }
            else
            {
                // 资源扣除失败回退宝石
                resMgr.addResource(ResourceType::kGem, GEM_COST);
            }
        }
        else
        {
            CCLOG("Not enough resources or gems");
        }
    });
    _panel->addChild(gemButton, 1); // ZOrder设为1，确保在上层

    // --- C. 关闭按钮 (红色) ---
    _closeButton = Button::create("icon/return_button.png");
    if (_closeButton->getContentSize().equals(Size::ZERO))
    {
        _closeButton = Button::create();
        _closeButton->ignoreContentAdaptWithSize(false);
        _closeButton->setContentSize(Size(btnWidth, btnHeight));
        _closeButton->setTitleText("关闭");
        _closeButton->setTitleFontSize(16);
        _closeButton->setTitleColor(Color3B::WHITE);

        // auto closeBg = LayerColor::create(Color4B(150, 0, 0, 200), btnWidth, btnHeight);
        // closeBg->setAnchorPoint(Vec2::ZERO);
        // closeBg->setPosition(Vec2::ZERO);
        // _closeButton->addChild(closeBg, -1);

        if (_closeButton->getTitleRenderer())
        {
            _closeButton->getTitleRenderer()->setPosition(Vec2(btnWidth / 2, btnHeight / 2));
        }
    } else {
        _closeButton->setScale(40.0f / _closeButton->getContentSize().width);
    }
    _closeButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    _closeButton->setPosition(Vec2(startX + btnWidth * 2, bottomY)); // 位置3：紧挨着
    _closeButton->setTouchEnabled(true);
    _closeButton->setPressedActionEnabled(true);
    _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });

    _panel->addChild(_closeButton);

    // ============================================================
    // 4. 训练士兵按钮 (仅兵营)
    // ============================================================
    if (isBarracks)
    {
        // 放置在 Y=100，位于文字和底部按钮中间
        _trainButton = Button::create();
        _trainButton->setTitleText("⚔ 训练士兵");
        _trainButton->setTitleFontSize(18);

        Size trainSize(200, 40);
        _trainButton->setContentSize(trainSize);
        _trainButton->setAnchorPoint(Vec2(0.5f, 0.5f));
        _trainButton->setPosition(Vec2(centerX, 100));
        _trainButton->setTouchEnabled(true);
        _trainButton->setPressedActionEnabled(true);
        _trainButton->addClickEventListener([this](Ref*) { onTrainClicked(); });

        // auto trainBg = LayerColor::create(Color4B(50, 100, 200, 200), trainSize.width, trainSize.height);
        // trainBg->setAnchorPoint(Vec2::ZERO);
        // trainBg->setPosition(Vec2::ZERO);
        // // trainBg->setTouchEnabled(false);
        // _trainButton->addChild(trainBg, -1);
        _panel->addChild(_trainButton);
    }
}

void BuildingUpgradeUI::updateUI()
{
    if (!_building)
        return;

    _titleLabel->setString(_building->getDisplayName());

    if (_building->isMaxLevel())
    {
        _levelLabel->setString(StringUtils::format("等级: %d (MAX)", _building->getLevel()));
        _costLabel->setString("已达最高等级");
        _timeLabel->setString("");
        if (_upgradeButton)
        {
            _upgradeButton->setEnabled(false);
            _upgradeButton->setTitleColor(Color3B::GRAY);
        }
    }
    else
    {
        _levelLabel->setString(StringUtils::format("等级: %d → %d", _building->getLevel(), _building->getLevel() + 1));

        std::string resName = getResourceTypeName(_building->getUpgradeCostType());
        _costLabel->setString(StringUtils::format("费用: %d %s", _building->getUpgradeCost(), resName.c_str()));

        _timeLabel->setString(StringUtils::format("时间: %s", formatTime(_building->getUpgradeTime()).c_str()));

        if (_upgradeButton)
        {
            _upgradeButton->setEnabled(true);
            _upgradeButton->setTitleColor(Color3B::WHITE);
        }

        bool hasResource =
            ResourceManager::getInstance().hasEnough(_building->getUpgradeCostType(), _building->getUpgradeCost());
        _costLabel->setTextColor(hasResource ? Color4B::GREEN : Color4B::RED);
    }

    _descLabel->setString(_building->getBuildingDescription());
}

void BuildingUpgradeUI::setPositionNearBuilding(BaseBuilding* building)
{
    if (!building)
        return;
    Vec2 worldPos = building->getParent()->convertToWorldSpace(building->getPosition());
    Vec2 offset(0, building->getContentSize().height * building->getScale() * 0.5f + 20);
    this->setPosition(worldPos + offset);
}

void BuildingUpgradeUI::show()
{
    this->setVisible(true);
    this->setScale(0.0f);
    auto scaleIn = EaseBackOut::create(ScaleTo::create(0.2f, 1.0f));
    this->runAction(scaleIn);
}

void BuildingUpgradeUI::hide()
{
    auto scaleOut = EaseBackIn::create(ScaleTo::create(0.15f, 0.0f));
    auto removeSelf = RemoveSelf::create();
    this->runAction(Sequence::create(scaleOut, removeSelf, nullptr));
}

void BuildingUpgradeUI::onUpgradeClicked()
{
    if (!_building)
        return;
    auto result = BuildingUpgradeService::getInstance().tryUpgrade(_building);

    if (result.success)
    {
        if (_resultCallback)
        {
            _resultCallback(true, _building->getLevel());
        }
        updateUI();
        onCloseClicked();
    }
    else
    {
        std::string hintMsg = result.message;
        switch (result.error)
        {
        case UpgradeError::kNoAvailableBuilder:
            hintMsg = "没有空闲的建筑工人！";
            break;
        case UpgradeError::kNotEnoughGold:
            hintMsg = "金币不足，无法升级！";
            break;
        case UpgradeError::kNotEnoughElixir:
            hintMsg = "圣水不足，无法升级！";
            break;
        default:
            break;
        }

        auto scene = Director::getInstance()->getRunningScene();
        if (scene)
        {
            auto label = Label::createWithSystemFont(hintMsg, "Arial", 24);
            label->setPosition(Director::getInstance()->getVisibleSize() / 2);
            label->setTextColor(Color4B::RED);
            scene->addChild(label, 10000);

            auto move = MoveBy::create(1.0f, Vec2(0, 50));
            auto fade = FadeOut::create(1.0f);
            auto seq  = Sequence::create(Spawn::create(move, fade, nullptr), RemoveSelf::create(), nullptr);
            label->runAction(seq);
        }

        CCLOG("升级失败: %s", hintMsg.c_str());
    }
}

void BuildingUpgradeUI::onCloseClicked()
{
    if (_closeCallback)
        _closeCallback();
    hide();
}

void BuildingUpgradeUI::onTrainClicked()
{
    if (!_building || _building->getBuildingType() != BuildingType::kArmy)
        return;

    auto barracks = dynamic_cast<ArmyBuilding*>(_building);
    if (!barracks)
        return;

    CCLOG("打开训练UI：%s", barracks->getDisplayName().c_str());

    if (_closeCallback)
        _closeCallback();

    auto trainingUI = TrainingUI::create(barracks);
    if (trainingUI)
    {
        auto scene = Director::getInstance()->getRunningScene();
        if (scene)
        {
            scene->addChild(trainingUI, 2000);
        }
    }

    hide();
}

std::string BuildingUpgradeUI::getResourceTypeName(ResourceType type) const
{
    switch (type)
    {
    case ResourceType::kGold:
        return "金币";
    case ResourceType::kElixir:
        return "圣水";
    case ResourceType::kGem:
        return "宝石";
    default:
        return "未知";
    }
}

std::string BuildingUpgradeUI::formatTime(int seconds) const
{
    if (seconds < 60)
    {
        return StringUtils::format("%d秒", seconds);
    }
    else if (seconds < 3600)
    {
        return StringUtils::format("%d分钟", seconds / 60);
    }
    else if (seconds < 86400)
    {
        return StringUtils::format("%d小时%d分", seconds / 3600, (seconds % 3600) / 60);
    }
    else
    {
        return StringUtils::format("%d天%d小时", seconds / 86400, (seconds % 86400) / 3600);
    }
}