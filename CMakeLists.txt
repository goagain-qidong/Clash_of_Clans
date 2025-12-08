cmake_minimum_required(VERSION 3.6)

set(APP_NAME HelloCpp)
project(${APP_NAME})

# --- 强制使用 UTF-8 编码 ---
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/wd4819)
endif()

if(XCODE)
    if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET)
        set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 8.0)
    endif()
endif()

if(NOT DEFINED BUILD_ENGINE_DONE)
    set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
    set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)
    include(CocosBuildSet)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)
endif()

set(GAME_SOURCE)
set(GAME_HEADER)

set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
if(APPLE OR WINDOWS)
    cocos_mark_multi_resources(common_res_files RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
endif()

# ---- 按照 Classes 目录组织自动收集文件 ----
file(GLOB_RECURSE APP_SRC        "${CMAKE_CURRENT_SOURCE_DIR}/Classes/App/*.cpp")
file(GLOB_RECURSE APP_HDR        "${CMAKE_CURRENT_SOURCE_DIR}/Classes/App/*.h")

file(GLOB_RECURSE SCENES_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Scenes/*.cpp")
file(GLOB_RECURSE SCENES_HDR     "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Scenes/*.h")

file(GLOB_RECURSE MANAGERS_SRC   "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Managers/*.cpp")
file(GLOB_RECURSE MANAGERS_HDR   "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Managers/*.h")

file(GLOB_RECURSE MODELS_SRC     "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Models/*.cpp")
file(GLOB_RECURSE MODELS_HDR     "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Models/*.h")

file(GLOB_RECURSE GRIDMAP_SRC    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/GridMap/*.cpp")
file(GLOB_RECURSE GRIDMAP_HDR    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/GridMap/*.h")

file(GLOB_RECURSE BUILDINGS_SRC  "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Buildings/*.cpp")
file(GLOB_RECURSE BUILDINGS_HDR  "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Buildings/*.h")

file(GLOB_RECURSE UNIT_SRC       "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Unit/*.cpp")
file(GLOB_RECURSE UNIT_HDR       "${CMAKE_CURRENT_SOURCE_DIR}/Classes/Unit/*.h")

file(GLOB_RECURSE UI_SRC         "${CMAKE_CURRENT_SOURCE_DIR}/Classes/UI/*.cpp")
file(GLOB_RECURSE UI_HDR         "${CMAKE_CURRENT_SOURCE_DIR}/Classes/UI/*.h")

list(APPEND GAME_SOURCE
    ${APP_SRC}
    ${SCENES_SRC}
    ${MANAGERS_SRC}
    ${MODELS_SRC}
    ${GRIDMAP_SRC}
    ${BUILDINGS_SRC}
    ${UNIT_SRC}
    ${UI_SRC}
)
list(APPEND GAME_HEADER
    ${APP_HDR}
    ${SCENES_HDR}
    ${MANAGERS_HDR}
    ${MODELS_HDR}
    ${GRIDMAP_HDR}
    ${BUILDINGS_HDR}
    ${UNIT_HDR}
    ${UI_HDR}
)

# ---- 平台特定 ----
if(ANDROID)
    set(APP_NAME MyGame)
    list(APPEND GAME_SOURCE proj.android/app/jni/hellocpp/main.cpp)
elseif(LINUX)
    list(APPEND GAME_SOURCE proj.linux/main.cpp)
elseif(WINDOWS)
    list(APPEND GAME_HEADER proj.win32/main.h proj.win32/resource.h)
    list(APPEND GAME_SOURCE proj.win32/main.cpp proj.win32/game.rc ${common_res_files})
elseif(APPLE)
    if(IOS)
        list(APPEND GAME_HEADER
            proj.ios_mac/ios/AppController.h
            proj.ios_mac/ios/RootViewController.h
        )
        set(APP_UI_RES
            proj.ios_mac/ios/LaunchScreen.storyboard
            proj.ios_mac/ios/LaunchScreenBackground.png
            proj.ios_mac/ios/Images.xcassets
        )
        list(APPEND GAME_SOURCE
            proj.ios_mac/ios/main.m
            proj.ios_mac/ios/AppController.mm
            proj.ios_mac/ios/RootViewController.mm
            proj.ios_mac/ios/Prefix.pch
            ${APP_UI_RES}
        )
    elseif(MACOSX)
        set(APP_UI_RES proj.ios_mac/mac/Icon.icns proj.ios_mac/mac/Info.plist)
        list(APPEND GAME_SOURCE proj.ios_mac/mac/main.cpp proj.ios_mac/mac/Prefix.pch ${APP_UI_RES})
    endif()
    list(APPEND GAME_SOURCE ${common_res_files})
endif()

set(all_code_files ${GAME_HEADER} ${GAME_SOURCE})
if(NOT ANDROID)
    add_executable(${APP_NAME} ${all_code_files})
else()
    add_library(${APP_NAME} SHARED ${all_code_files})
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos/platform/android ${ENGINE_BINARY_PATH}/cocos/platform)
    target_link_libraries(${APP_NAME} -Wl,--whole-archive cpp_android_spec -Wl,--no-whole-archive)
endif()

target_link_libraries(${APP_NAME} cocos2d)

# 统一包含路径
target_include_directories(${APP_NAME}
    PRIVATE Classes
    PRIVATE Classes/App
    PRIVATE Classes/Buildings
    PRIVATE Classes/GridMap
    PRIVATE Classes/Managers
    PRIVATE Classes/Models
    PRIVATE Classes/Scenes
    PRIVATE Classes/Unit
    PRIVATE Classes/UI
    PRIVATE ${COCOS2DX_ROOT_PATH}/cocos/audio/include/
)

setup_cocos_app_config(${APP_NAME})
if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
    if(MACOSX)
        set_xcode_property(${APP_NAME} INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist")
    elseif(IOS)
        set_xcode_property(${APP_NAME} INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/ios/Info.plist")
        set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    endif()
elseif(WINDOWS)
    cocos_copy_target_dll(${APP_NAME})
endif()

if(LINUX OR WINDOWS)
    cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
    cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
endif()