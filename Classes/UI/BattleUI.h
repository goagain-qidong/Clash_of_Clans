/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleUI.h
 * File Function: 战斗界面 - 负责管理游戏中的战斗相关UI
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef BATTLE_UI_H_
#define BATTLE_UI_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "Unit/UnitTypes.h"

/**
 * @class BattleUI
 * @brief 战斗UI层 - 管理战斗相关的用户界面
 */
class BattleUI : public cocos2d::Layer
{
public:
    /**
     * @brief 创建战斗UI
     * @return BattleUI* UI指针
     */
    static BattleUI* create();

    virtual bool init() override;

    /** @brief 设置结束战斗回调 */
    void setEndBattleCallback(const std::function<void()>& callback);

    /** @brief 设置返回回调 */
    void setReturnCallback(const std::function<void()>& callback);

    /** @brief 设置部队选择回调 */
    void setTroopSelectionCallback(const std::function<void(UnitType)>& callback);

    /** @brief 设置部队取消选择回调 */
    void setTroopDeselectionCallback(const std::function<void()>& callback);

    /**
     * @brief 更新状态文本
     * @param text 状态文本
     * @param color 颜色
     */
    void updateStatus(const std::string& text, const cocos2d::Color4B& color);

    /**
     * @brief 更新计时器
     * @param remainingTime 剩余时间
     */
    void updateTimer(int remainingTime);

    /**
     * @brief 更新星星数
     * @param stars 星星数
     */
    void updateStars(int stars);

    /**
     * @brief 更新摧毁百分比
     * @param percent 百分比
     */
    void updateDestruction(int percent);

    /**
     * @brief 更新部队数量
     */
    void updateTroopCounts(int barbarianCount, int archerCount, int giantCount, int goblinCount, int wallBreakerCount);

    /**
     * @brief 设置回放模式
     * @param isReplay 是否为回放
     */
    void setReplayMode(bool isReplay);

    /** @brief 显示/隐藏战斗HUD */
    void showBattleHUD(bool visible);

    /** @brief 显示/隐藏部队按钮 */
    void showTroopButtons(bool visible);

    /** @brief 显示/隐藏返回按钮 */
    void showReturnButton(bool visible);

    /**
     * @brief 显示结果面板
     */
    void showResultPanel(int stars, int destructionPercent, int goldLooted, int elixirLooted, int trophyChange,
                         bool isReplayMode);

    /**
     * @brief 高亮部队按钮
     * @param type 单位类型
     */
    void highlightTroopButton(UnitType type);

    /** @brief 清除部队高亮 */
    void clearTroopHighlight();

    /**
     * @brief 设置结束战斗按钮文本
     * @param text 文本
     */
    void setEndBattleButtonText(const std::string& text);

    /** @brief 获取选中的单位类型 */
    UnitType getSelectedUnitType() const { return _selectedUnitType; }

    /** @brief 是否有选中的单位 */
    bool hasSelectedUnit() const { return _hasSelectedUnit; }

private:
    void setupTopBar();       ///< 设置顶部栏
    void setupBottomButtons(); ///< 设置底部按钮
    void setupTroopButtons();  ///< 设置部队按钮

    cocos2d::Node* createTroopCard(UnitType type, const std::string& iconPath, const std::string& name);
    void updateTroopCardCount(UnitType type, int count);
    void onTroopCardClicked(UnitType type);

    cocos2d::Size _visibleSize;       ///< 可视区域大小
    bool _isReplayMode = false;       ///< 是否为回放模式
    int _starsEarned = 0;             ///< 获得的星星数

    UnitType _selectedUnitType = UnitType::kBarbarian;  ///< 选中的单位类型
    bool _hasSelectedUnit = false;    ///< 是否有选中的单位

    cocos2d::Label* _statusLabel = nullptr;       ///< 状态标签
    cocos2d::Label* _timerLabel = nullptr;        ///< 计时器标签
    cocos2d::Label* _starsLabel = nullptr;        ///< 星星标签
    cocos2d::Label* _destructionLabel = nullptr;  ///< 摧毁百分比标签
    cocos2d::ui::Button* _endBattleButton = nullptr;  ///< 结束战斗按钮
    cocos2d::ui::Button* _returnButton = nullptr;     ///< 返回按钮

    cocos2d::Node* _troopPanel = nullptr;  ///< 部队面板

    cocos2d::Node* _barbarianCard = nullptr;     ///< 野蛮人卡片
    cocos2d::Node* _archerCard = nullptr;        ///< 弓箭手卡片
    cocos2d::Node* _giantCard = nullptr;         ///< 巨人卡片
    cocos2d::Node* _goblinCard = nullptr;        ///< 哥布林卡片
    cocos2d::Node* _wallBreakerCard = nullptr;   ///< 炸弹人卡片

    cocos2d::Sprite* _selectionFrame = nullptr;  ///< 选中框

    cocos2d::ui::Button* _barbarianButton = nullptr;   ///< 野蛮人按钮
    cocos2d::ui::Button* _archerButton = nullptr;      ///< 弓箭手按钮
    cocos2d::ui::Button* _giantButton = nullptr;       ///< 巨人按钮
    cocos2d::ui::Button* _goblinButton = nullptr;      ///< 哥布林按钮
    cocos2d::ui::Button* _wallBreakerButton = nullptr; ///< 炸弹人按钮
    cocos2d::Label* _barbarianCountLabel = nullptr;    ///< 野蛮人数量
    cocos2d::Label* _archerCountLabel = nullptr;       ///< 弓箭手数量
    cocos2d::Label* _giantCountLabel = nullptr;        ///< 巨人数量
    cocos2d::Label* _goblinCountLabel = nullptr;       ///< 哥布林数量
    cocos2d::Label* _wallBreakerCountLabel = nullptr;  ///< 炸弹人数量

    std::function<void()> _onEndBattle;           ///< 结束战斗回调
    std::function<void()> _onReturn;              ///< 返回回调
    std::function<void(UnitType)> _onTroopSelected;   ///< 部队选择回调
    std::function<void()> _onTroopDeselected;     ///< 部队取消选择回调
};

#endif // BATTLE_UI_H_