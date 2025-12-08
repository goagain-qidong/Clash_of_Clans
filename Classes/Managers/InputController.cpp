#include "InputController.h"

USING_NS_CC;

bool InputController::init()
{
    if (!Node::init())
    {
        return false;
    }
    
    setupTouchListener();
    setupMouseListener();
    setupKeyboardListener();
    
    return true;
}

void InputController::setupTouchListener()
{
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    
    touchListener->onTouchBegan = CC_CALLBACK_2(InputController::handleTouchBegan, this);
    touchListener->onTouchMoved = CC_CALLBACK_2(InputController::handleTouchMoved, this);
    touchListener->onTouchEnded = CC_CALLBACK_2(InputController::handleTouchEnded, this);
    touchListener->onTouchCancelled = CC_CALLBACK_2(InputController::handleTouchCancelled, this);
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
}

void InputController::setupMouseListener()
{
    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseScroll = CC_CALLBACK_1(InputController::handleMouseScroll, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
}

void InputController::setupKeyboardListener()
{
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = CC_CALLBACK_2(InputController::handleKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);
}

bool InputController::handleTouchBegan(Touch* touch, Event* event)
{
    if (!_inputEnabled)
        return false;
    
    _lastTouchPos = touch->getLocation();
    
    if (_onTouchBegan)
    {
        return _onTouchBegan(touch, event);
    }
    
    return true;
}

void InputController::handleTouchMoved(Touch* touch, Event* event)
{
    if (!_inputEnabled)
        return;
    
    if (_onTouchMoved)
    {
        _onTouchMoved(touch, event);
    }
}

void InputController::handleTouchEnded(Touch* touch, Event* event)
{
    if (!_inputEnabled)
        return;
    
    if (_onTouchEnded)
    {
        _onTouchEnded(touch, event);
    }
}

void InputController::handleTouchCancelled(Touch* touch, Event* event)
{
    handleTouchEnded(touch, event);
}

void InputController::handleMouseScroll(Event* event)
{
    if (!_inputEnabled)
        return;
    
    EventMouse* mouseEvent = dynamic_cast<EventMouse*>(event);
    if (mouseEvent && _onMouseScroll)
    {
        float scrollY = mouseEvent->getScrollY();
        Vec2 mousePos = Vec2(mouseEvent->getCursorX(), mouseEvent->getCursorY());
        _onMouseScroll(scrollY, mousePos);
    }
}

void InputController::handleKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (!_inputEnabled)
        return;
    
    if (_onKeyPressed)
    {
        _onKeyPressed(keyCode);
    }
}

void InputController::setInputEnabled(bool enabled)
{
    _inputEnabled = enabled;
}
