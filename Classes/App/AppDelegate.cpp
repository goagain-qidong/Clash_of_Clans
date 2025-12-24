/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

// Visual Leak Detector - 内存泄漏检测（仅 Debug 模式生效）
// 安装方法：https://github.com/KindDragon/vld/releases
// 安装后需要在项目属性中添加 Include 和 Lib 路径
#if defined(_DEBUG) && defined(_WIN32)
    // 如果已安装 VLD，取消下面这行的注释
    // #include <vld.h>
#endif

#include "AppDelegate.h"
#include "AccountSelectScene.h"
#include "DraggableMapScene.h"
#include "HelloWorldScene.h"
#include "Managers/AccountManager.h"
#include "Managers/ResourceManager.h" // 新增：包含资源管理器头文件
#include "Managers/UpgradeManager.h"  // 新增：包含升级管理器头文件
// #define USE_AUDIO_ENGINE 1
#if USE_AUDIO_ENGINE
#include "audio/include/AudioEngine.h"
using namespace cocos2d::experimental;
#endif
USING_NS_CC;
static cocos2d::Size designResolutionSize = cocos2d::Size(2180, 1480);
static cocos2d::Size smallResolutionSize = cocos2d::Size(480, 320);
static cocos2d::Size mediumResolutionSize = cocos2d::Size(1024, 768);
static cocos2d::Size largeResolutionSize = cocos2d::Size(2048, 1536);
AppDelegate::AppDelegate() {}
AppDelegate::~AppDelegate()
{
#if USE_AUDIO_ENGINE
    AudioEngine::end();
#endif
    // 清理单例以防止内存泄漏
    ResourceManager::destroyInstance();
    UpgradeManager::destroyInstance();
}
// if you want a different context, modify the value of glContextAttrs
// it will affect all platforms
void AppDelegate::initGLContextAttrs()
{
    // set OpenGL context attributes: red,green,blue,alpha,depth,stencil,multisamplesCount
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8, 0};
    GLView::setGLContextAttrs(glContextAttrs);
}
// if you want to use the package manager to install more packages,
// don't modify or remove this function
static int register_all_packages()
{
    return 0; // flag for packages manager
}
bool AppDelegate::applicationDidFinishLaunching()
{
    // 1. 获取导演
    auto director = Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview)
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
        // Android 平台：使用 GLViewImpl::create
        glview = GLViewImpl::create("部落冲突(同济大学版)");
        director->setOpenGLView(glview);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
        // Windows/Linux/Mac 平台：使用 createWithRect 支持自定义窗口尺寸
        glview = GLViewImpl::createWithRect("部落冲突(同济大学版)",        // 标题
                                            Rect(0, 0, 2560, 1440), // 初始位置和大小(2K)
                                            1.0f,                   // 缩放系数
                                            true                    // 设为 true 表示可以用鼠标拉伸窗口
        );
        director->setOpenGLView(glview);
#else
        // 其他平台：使用默认创建方式
        glview = GLView::create("部落冲突(同济大学版)");
        director->setOpenGLView(glview);
#endif
    }
    // 2. 开启调试数据显示 (FPS等)
    //director->setDisplayStats(true);
    // 3. 设置帧率
    director->setAnimationInterval(1.0f / 60);
    // 4. 不要用 NO_BORDER，因为你现在允许乱拉窗口了，NO_BORDER 会切掉你的 UI
    // 推荐用 FIXED_HEIGHT：高度固定，宽度视野变大 (适合 COC 类游戏)
    glview->setDesignResolutionSize(1280, 720, ResolutionPolicy::FIXED_HEIGHT);
#if 0
    if (frameSize.height > mediumResolutionSize.height)
    {        
        director->setContentScaleFactor(MIN(largeResolutionSize.height/designResolutionSize.height, largeResolutionSize.width/designResolutionSize.width));
    }
    else if (frameSize.height > smallResolutionSize.height)
    {        
        director->setContentScaleFactor(MIN(mediumResolutionSize.height/designResolutionSize.height, mediumResolutionSize.width/designResolutionSize.width));
    }
    else
    {        
        director->setContentScaleFactor(MIN(smallResolutionSize.height/designResolutionSize.height, smallResolutionSize.width/designResolutionSize.width));
    }
#endif
    register_all_packages();
    // 新增：初始化资源管理器并设置初始资源
    CCLOG("Initializing Resource Manager...");
    auto resourceManager = &ResourceManager::getInstance();
    resourceManager->init();
    CCLOG("Resource Manager initialized successfully");
    // 设置初始资源（游戏开始送2000金币，1000圣水）
    resourceManager->addResource(ResourceType::kGold, 2000);
    resourceManager->addResource(ResourceType::kElixir, 1000);
    CCLOG("Initial resources set: Gold=2000, Elixir=1000");
    // Initialize account system (load from storage)
    AccountManager::getInstance().initialize();
    // Always go to account selection first
    Scene* scene = AccountSelectScene::createScene();
    director->runWithScene(scene);
    return true;
}
// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();
#if USE_AUDIO_ENGINE
    AudioEngine::pauseAll();
#endif
}
// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();
#if USE_AUDIO_ENGINE
    AudioEngine::resumeAll();
#endif
}
