/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     InputController.h
 * File Function: 输入控制器头文件
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __INPUT_CONTROLLER_H__
#define __INPUT_CONTROLLER_H__

#include "cocos2d.h"
#include <functional>

/**
 * @class InputController
 * @brief 输入控制器 - 负责所有输入事件的处理和优先级管理
 * 
 * 职责：
 * - 触摸事件处理（onTouchBegan/Moved/Ended）
 * - 鼠标滚轮缩放
 * - 键盘快捷键（ESC 取消建造）
 * - 输入优先级管理（UI > 建筑建造 > 地图操作）
 */
class InputController : public cocos2d::Node
{
public:
    CREATE_FUNC(InputController);
    
    virtual bool init() override;
    
    // ==================== 回调类型 ====================
    
    using TouchCallback = std::function<bool(cocos2d::Touch*, cocos2d::Event*)>;
    using TouchMoveCallback = std::function<void(cocos2d::Touch*, cocos2d::Event*)>;
    using ScrollCallback = std::function<void(float scrollY, cocos2d::Vec2 mousePos)>;
    using KeyCallback = std::function<void(cocos2d::EventKeyboard::KeyCode)>;
    
    // ==================== 回调设置 ====================
    
    /**
     * @brief 设置触摸开始回调
     * @param callback 返回true表示消费该事件
     */
    void setOnTouchBegan(const TouchCallback& callback) { _onTouchBegan = callback; }
    
    /**
     * @brief 设置触摸移动回调
     */
    void setOnTouchMoved(const TouchMoveCallback& callback) { _onTouchMoved = callback; }
    
    /**
     * @brief 设置触摸结束回调
     */
    void setOnTouchEnded(const TouchMoveCallback& callback) { _onTouchEnded = callback; }
    
    /**
     * @brief 设置鼠标滚轮回调
     * @param callback scrollY > 0 向上，< 0 向下
     */
    void setOnMouseScroll(const ScrollCallback& callback) { _onMouseScroll = callback; }
    
    /**
     * @brief 设置键盘按键回调
     */
    void setOnKeyPressed(const KeyCallback& callback) { _onKeyPressed = callback; }
    
    /**
     * @brief 启用/禁用输入
     */
    void setInputEnabled(bool enabled);
    
private:
    cocos2d::Vec2 _lastTouchPos;
    bool _inputEnabled = true;
    
    // ==================== 回调 ====================
    TouchCallback _onTouchBegan;
    TouchMoveCallback _onTouchMoved;
    TouchMoveCallback _onTouchEnded;
    ScrollCallback _onMouseScroll;
    KeyCallback _onKeyPressed;
    
    // ==================== 内部方法 ====================
    void setupTouchListener();
    void setupMouseListener();
    void setupKeyboardListener();
    
    bool handleTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void handleTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void handleTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void handleTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);
    void handleMouseScroll(cocos2d::Event* event);
    void handleKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
};

#endif // __INPUT_CONTROLLER_H__
