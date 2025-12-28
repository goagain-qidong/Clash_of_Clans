/****************************************************************
* Project Name:  Clash_of_Clans
* File Name:     BuildingUpgradeUI.cpp
* File Function: 通用建筑升级界面实现
* Author:        刘相成、薛毓哲
* Update Date:   2025/12/24
* License:       MIT License
****************************************************************/

#include "BuildingUpgradeUI.h"
#include "ArmyBuilding.h"
#include "ArmyCampBuilding.h"
#include "Audio/AudioManager.h"
#include "Managers/UpgradeManager.h"
#include "Managers/BuildingManager.h"
#include "Managers/TroopInventory.h"
#include "ResourceManager.h"
#include "SceneUIController.h"
#include "Scenes/DraggableMapScene.h"
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
    bool  isArmyCamp  = (_building && _building->getBuildingType() == BuildingType::kArmyCamp);
    float panelHeight = (isBarracks || isArmyCamp) ? 340.0f : 260.0f;

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
    // 关闭按钮 (左上角)
    // ============================================================
    _closeButton = Button::create("icon/return_button.png");
    if (_closeButton->getContentSize().equals(Size::ZERO))
    {
        _closeButton = Button::create();
        _closeButton->ignoreContentAdaptWithSize(false);
        _closeButton->setContentSize(Size(40, 40));
        _closeButton->setTitleText("X");
        _closeButton->setTitleFontSize(20);
        _closeButton->setTitleColor(Color3B::WHITE);

        auto closeBg = LayerColor::create(Color4B(200, 0, 0, 200), 40, 40);
        closeBg->setAnchorPoint(Vec2::ZERO);
        closeBg->setPosition(Vec2::ZERO);
        _closeButton->addChild(closeBg, -1);

        if (_closeButton->getTitleRenderer())
        {
            _closeButton->getTitleRenderer()->setPosition(Vec2(20, 20));
        }
    }
    else
    {
        _closeButton->setScale(40.0f / _closeButton->getContentSize().width);
    }
    _closeButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    _closeButton->setPosition(Vec2(20, panelHeight - 20)); // 左上角位置
    _closeButton->setTouchEnabled(true);
    _closeButton->setPressedActionEnabled(true);
    _closeButton->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        onCloseClicked();
    });
    _panel->addChild(_closeButton);

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

    // 提示标签 (工人不足)
    auto builderHint = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    builderHint->setName("builderHint");
    builderHint->setPosition(Vec2(centerX, bottomY + 35));
    builderHint->setTextColor(Color4B::RED);
    _panel->addChild(builderHint);

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
    _upgradeButton->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        onUpgradeClicked();
    });

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

    // 重新写一遍点击逻辑，确保无误
    gemButton->addClickEventListener([this](Ref* sender) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
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

            auto scene = Director::getInstance()->getRunningScene();
            if (scene)
            {
                auto label = Label::createWithSystemFont("没有空闲的建筑工人！", "Arial", 24);
                label->setPosition(Director::getInstance()->getVisibleSize() / 2);
                label->setTextColor(Color4B::RED);
                scene->addChild(label, 10000);

                auto move = MoveBy::create(1.0f, Vec2(0, 50));
                auto fade = FadeOut::create(1.0f);
                auto seq  = Sequence::create(Spawn::create(move, fade, nullptr), RemoveSelf::create(), nullptr);
                label->runAction(seq);
            }
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
            // 🔴 修复：显示资源或宝石不足的提示
            std::string hintMsg;
            if (!hasGem)
            {
                hintMsg = StringUtils::format("宝石不足！需要：%d，当前：%d",
                                              GEM_COST, resMgr.getResourceCount(ResourceType::kGem));
            }
            else
            {
                std::string resName = (costType == ResourceType::kGold) ? "金币" : "圣水";
                hintMsg = StringUtils::format("%s不足！需要：%d，当前：%d",
                                              resName.c_str(), cost, resMgr.getResourceCount(costType));
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
            CCLOG("Not enough resources or gems: %s", hintMsg.c_str());
        }
    });
    _panel->addChild(gemButton, 1); // ZOrder设为1，确保在上层

    // --- C. 移动按钮 (蓝色) ---
    _moveButton = Button::create();
    _moveButton->setContentSize(Size(btnWidth, btnHeight));
    _moveButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    _moveButton->setPosition(Vec2(startX + btnWidth * 2, bottomY)); // 位置3：紧挨着
    _moveButton->setTitleText("移动");
    _moveButton->setTitleFontSize(16);
    _moveButton->setTitleColor(Color3B::WHITE);
    _moveButton->setTouchEnabled(true);
    _moveButton->setPressedActionEnabled(true);
    _moveButton->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        onMoveClicked();
    });
    _panel->addChild(_moveButton);

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
        _trainButton->addClickEventListener([this](Ref*) {
            AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
            onTrainClicked();
        });

        _panel->addChild(_trainButton);
    }
    
    // ============================================================
    // 5. 军队详情按钮 (仅军营)
    // ============================================================
    if (isArmyCamp)
    {
        // 放置在 Y=100，位于文字和底部按钮中间
        _armyCampButton = Button::create();
        _armyCampButton->setTitleText("🪖 军队详情");
        _armyCampButton->setTitleFontSize(18);

        Size armySize(200, 40);
        _armyCampButton->setContentSize(armySize);
        _armyCampButton->setAnchorPoint(Vec2(0.5f, 0.5f));
        _armyCampButton->setPosition(Vec2(centerX, 100));
        _armyCampButton->setTouchEnabled(true);
        _armyCampButton->setPressedActionEnabled(true);
        _armyCampButton->addClickEventListener([this](Ref*) {
            AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
            onArmyCampClicked();
        });

        _panel->addChild(_armyCampButton);
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

    // 更新工人不足提示
    auto builderHint = dynamic_cast<Label*>(_panel->getChildByName("builderHint"));
    if (builderHint)
    {
        if (!_building->isMaxLevel() && UpgradeManager::getInstance()->getAvailableBuilders() <= 0)
        {
            builderHint->setString("工人不足");
        }
        else
        {
            builderHint->setString("");
        }
    }
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
        case UpgradeError::kNotEnoughGem:
            hintMsg = "宝石不足，无法升级！";
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

void BuildingUpgradeUI::onMoveClicked()
{
    if (!_building)
        return;

    CCLOG("开始移动建筑：%s", _building->getDisplayName().c_str());

    // 获取当前场景
    auto scene = dynamic_cast<DraggableMapScene*>(Director::getInstance()->getRunningScene());
    if (!scene)
    {
        CCLOG("无法获取场景，移动建筑失败！");
        return;
    }

    // 获取 BuildingManager
    auto manager = scene->getBuildingManager();
    if (!manager)
    {
        CCLOG("无法获取 BuildingManager，移动建筑失败！");
        return;
    }

    // 开始移动建筑
    manager->startMovingBuilding(_building);

    // 关闭升级UI
    if (_closeCallback)
        _closeCallback();
    hide();
}

void BuildingUpgradeUI::onArmyCampClicked()
{
    if (!_building || _building->getBuildingType() != BuildingType::kArmyCamp)
        return;

    auto armyCamp = dynamic_cast<ArmyCampBuilding*>(_building);
    if (!armyCamp)
        return;

    CCLOG("打开军队详情：%s", armyCamp->getDisplayName().c_str());

    // 关闭当前UI
    if (_closeCallback)
        _closeCallback();
    hide();

    // 创建军队详情UI
    auto scene = Director::getInstance()->getRunningScene();
    if (!scene)
        return;

    // 创建半透明背景
    auto bgLayer = LayerColor::create(Color4B(0, 0, 0, 180));
    bgLayer->setName("ArmyDetailBG");
    scene->addChild(bgLayer, 9999);

    // 创建面板
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto panel = Layout::create();
    panel->setContentSize(Size(400, 500));
    panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    panel->setBackGroundColor(Color3B(40, 40, 60));
    panel->setBackGroundColorOpacity(240);
    panel->setAnchorPoint(Vec2(0.5f, 0.5f));
    panel->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
    bgLayer->addChild(panel);

    // 标题
    auto title = Label::createWithSystemFont("军队详情", "Microsoft YaHei", 24);
    title->setPosition(Vec2(200, 460));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    // 关闭按钮
    auto closeBtn = Button::create("icon/return_button.png");
    if (closeBtn->getContentSize().equals(Size::ZERO))
    {
        closeBtn = Button::create();
        closeBtn->setContentSize(Size(40, 40));
        closeBtn->setTitleText("X");
        closeBtn->setTitleFontSize(20);
        closeBtn->setTitleColor(Color3B::WHITE);
    }
    else
    {
        closeBtn->setScale(40.0f / closeBtn->getContentSize().width);
    }
    closeBtn->setPosition(Vec2(20, 480));
    closeBtn->addClickEventListener([bgLayer](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        bgLayer->removeFromParent();
    });
    panel->addChild(closeBtn);

    // 获取士兵库存
    auto& inventory = TroopInventory::getInstance();
    const auto& allTroops = inventory.getAllTroops();

    // 显示容量信息
    int totalPopulation = inventory.getTotalPopulation();
    int maxCapacity = armyCamp->getHousingSpace();
    auto capacityLabel = Label::createWithSystemFont(
        StringUtils::format("容纳人口: %d / %d", totalPopulation, maxCapacity),
        "Microsoft YaHei", 18);
    capacityLabel->setPosition(Vec2(200, 410));
    capacityLabel->setTextColor(totalPopulation > maxCapacity ? Color4B::RED : Color4B::GREEN);
    panel->addChild(capacityLabel);

    // 兵种名称映射
    std::map<UnitType, std::string> troopNames = {
        {UnitType::kBarbarian, "野蛮人"},
        {UnitType::kArcher, "弓箭手"},
        {UnitType::kGiant, "巨人"},
        {UnitType::kGoblin, "哥布林"},
        {UnitType::kWallBreaker, "炸弹人"}
    };

    // 显示士兵列表
    float yPos = 360;
    int troopIndex = 0;
    
    for (const auto& pair : allTroops)
    {
        if (pair.second <= 0)
            continue;

        UnitType type = pair.first;
        int count = pair.second;
        
        std::string troopName = "未知兵种";
        auto it = troopNames.find(type);
        if (it != troopNames.end())
        {
            troopName = it->second;
        }

        // 兵种行背景
        auto rowBg = LayerColor::create(
            troopIndex % 2 == 0 ? Color4B(60, 60, 80, 200) : Color4B(50, 50, 70, 200),
            380, 50);
        rowBg->setPosition(Vec2(10, yPos - 40));
        panel->addChild(rowBg);

        // 兵种名称
        auto nameLabel = Label::createWithSystemFont(troopName, "Microsoft YaHei", 18);
        nameLabel->setAnchorPoint(Vec2(0, 0.5f));
        nameLabel->setPosition(Vec2(30, yPos - 15));
        nameLabel->setTextColor(Color4B::WHITE);
        panel->addChild(nameLabel);

        // 数量
        auto countLabel = Label::createWithSystemFont(
            StringUtils::format("x %d", count),
            "Microsoft YaHei", 20);
        countLabel->setAnchorPoint(Vec2(1, 0.5f));
        countLabel->setPosition(Vec2(370, yPos - 15));
        countLabel->setTextColor(Color4B::YELLOW);
        panel->addChild(countLabel);

        yPos -= 60;
        troopIndex++;
    }

    // 如果没有士兵，显示提示
    if (allTroops.empty() || troopIndex == 0)
    {
        auto emptyLabel = Label::createWithSystemFont(
            "暂无士兵\n前往兵营训练士兵",
            "Microsoft YaHei", 20);
        emptyLabel->setPosition(Vec2(200, 250));
        emptyLabel->setTextColor(Color4B::GRAY);
        emptyLabel->setAlignment(TextHAlignment::CENTER);
        panel->addChild(emptyLabel);
    }

    // 底部说明
    auto hintLabel = Label::createWithSystemFont(
        "提示：前往兵营训练更多士兵",
        "Microsoft YaHei", 14);
    hintLabel->setPosition(Vec2(200, 30));
    hintLabel->setTextColor(Color4B(200, 200, 200, 255));
    panel->addChild(hintLabel);
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