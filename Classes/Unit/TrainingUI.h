/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TrainingUI.h
 * File Function: 训练小兵UI界面
 * Author:        薛毓哲
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#ifndef TRAINING_UI_H_
#define TRAINING_UI_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "UnitTypes.h"

class ArmyBuilding;

/**
 * @class TrainingUI
 * @brief 训练小兵的UI界面，点击兵营时弹出
 */
class TrainingUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建训练UI
     * @param barracks 兵营建筑指针
     * @return TrainingUI* 训练UI指针
     */
    static TrainingUI* create(ArmyBuilding* barracks);

    /**
     * @brief 初始化
     * @param barracks 兵营建筑指针
     * @return bool 初始化是否成功
     */
    virtual bool init(ArmyBuilding* barracks);

    /** @brief 显示UI */
    void show();

    /** @brief 隐藏UI */
    void hide();

    /**
     * @brief 每帧更新
     * @param dt 帧时间间隔
     */
    virtual void update(float dt) override;

private:
    void setupUI();  ///< 设置UI界面

    /**
     * @brief 创建兵种卡片
     * @param scrollView 滚动视图
     * @param unitType 兵种类型
     * @param name 兵种名称
     * @param cost 训练费用
     * @param housingSpace 占用人口数
     */
    void createUnitCard(cocos2d::ui::ListView* scrollView, UnitType unitType, const std::string& name, int cost,
                        int housingSpace);

    void updatePopulationDisplay();  ///< 更新人口显示

    /**
     * @brief 点击训练按钮
     * @param unitType 要训练的兵种
     */
    void onTrainButtonClicked(UnitType unitType);

    void onCloseClicked();  ///< 点击关闭按钮

    /**
     * @brief 获取兵种名称
     * @param type 单位类型
     * @return std::string 兵种名称
     */
    std::string getUnitName(UnitType type) const;

private:
    ArmyBuilding* _barracks = nullptr;             ///< 兵营建筑
    cocos2d::ui::Layout* _panel = nullptr;         ///< 背景面板
    cocos2d::Label* _titleLabel = nullptr;         ///< 标题
    cocos2d::ui::Button* _closeButton = nullptr;   ///< 关闭按钮

    int _lastTroopCount = 0;     ///< 上次部队数量
    int _lastTroopCapacity = 0;  ///< 上次部队容量
};

#endif // TRAINING_UI_H_
