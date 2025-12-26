cmake_minimum_required(VERSION 3.10)

set(APP_NAME Sigeluguanshou)

project(${APP_NAME})

set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)

include(CocosBuildSet)
if(NOT USE_COCOS_PREBUILT)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)
endif()

set(GAME_SOURCE)
set(GAME_HEADER)

set(GAME_RES_FOLDER
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
    )
if(APPLE OR WINDOWS)
    cocos_mark_multi_resources(common_res_files RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
endif()

include_directories(
        Classes
        ${COCOS2DX_ROOT_PATH}/cocos/audio/include/
)

list(APPEND GAME_SOURCE
     Classes/AppDelegate.cpp
     Classes/HelloWorldScene.cpp
     Classes/MenuScene.cpp
     Classes/HouseScene.cpp
     Classes/GameScene.cpp
     Classes/BeachScene.cpp
     Classes/Player.cpp
     Classes/MapLayer.cpp
     Classes/FarmManager.cpp
     Classes/FishingLayer.cpp
     Classes/InventoryManager.cpp
     Classes/InventoryUI.cpp
     Classes/MarketState.cpp
     Classes/MarketUI.cpp
     Classes/MineScene.cpp
     Classes/MineLayer.cpp
     Classes/MiningManager.cpp
     Classes/Monster.cpp
     Classes/Slime.cpp
     Classes/Zombie.cpp
     Classes/Weapon.cpp
     Classes/TreasureChest.cpp
     Classes/ElevatorUI.cpp
     Classes/SaveManager.cpp
     Classes/WeatherManager.cpp
     Classes/EnergyBar.cpp
     Classes/StorageChest.cpp
     Classes/ShippingBin.cpp
     Classes/QuantityPopup.cpp
     Classes/SkillManager.cpp
     Classes/SkillTreeUI.cpp
     Classes/DialogueBox.cpp
     Classes/Npc.cpp
     Classes/TimeManager.cpp
     )
list(APPEND GAME_HEADER
     Classes/AppDelegate.h
     Classes/HelloWorldScene.h
     Classes/MenuScene.h
     Classes/HouseScene.h
     Classes/GameScene.h
     Classes/BeachScene.h
     Classes/Player.h
     Classes/MapLayer.h
     Classes/FarmManager.h
     Classes/FishingLayer.h
     Classes/InventoryManager.h
     Classes/InventoryUI.h
     Classes/MarketState.h
     Classes/MarketUI.h
     Classes/MineScene.h
     Classes/MineLayer.h
     Classes/MiningManager.h
     Classes/Monster.h
     Classes/Slime.h
     Classes/Zombie.h
     Classes/Weapon.h
     Classes/TreasureChest.h
     Classes/ElevatorUI.h
     Classes/SaveManager.h
     Classes/WeatherManager.h
     Classes/EnergyBar.h
     Classes/StorageChest.h
     Classes/ShippingBin.h
     Classes/QuantityPopup.h
     Classes/SkillManager.h
     Classes/SkillTreeUI.h
     Classes/DialogueBox.h
     Classes/Npc.h
     Classes/TimeManager.h
     )

if(ANDROID)
    set(APP_NAME MyGame)
    list(APPEND GAME_SOURCE
         proj.android/app/jni/hellocpp/main.cpp
         )
elseif(LINUX)
    list(APPEND GAME_SOURCE
         proj.linux/main.cpp
         )
elseif(WINDOWS)
    list(APPEND GAME_HEADER
         proj.win32/main.h
         proj.win32/resource.h
         )
    list(APPEND GAME_SOURCE
         proj.win32/main.cpp
         proj.win32/game.rc
         ${common_res_files}
         )
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
        set(APP_UI_RES
            proj.ios_mac/mac/Icon.icns
            proj.ios_mac/mac/Info.plist
            )
        list(APPEND GAME_SOURCE
             proj.ios_mac/mac/main.cpp
             proj.ios_mac/mac/Prefix.pch
             ${APP_UI_RES}
             )
    endif()
    list(APPEND GAME_SOURCE ${common_res_files})
endif()

set(APP_SRC
    ${GAME_SOURCE}
    ${GAME_HEADER}
    )
cocos_build_app(${APP_NAME}
                APP_SRC "${APP_SRC}"
                DEPEND_COMMON_LIBS "cocos2d"
                DEPEND_ANDROID_LIBS "cocos2d_android"
                )

if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
    if(MACOSX)
        set_target_properties(${APP_NAME} PROPERTIES
                              MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist"
                              )
    elseif(IOS)
        cocos_pak_xcode(${APP_NAME} INFO_PLIST "iOSBundleInfo.plist.in")
        set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    endif()
elseif(WINDOWS)
    cocos_copy_target_dll(${APP_NAME} COPY_TO ${APP_BIN_DIR})
endif()

if(LINUX OR WINDOWS)
    cocos_copy_target_res(${APP_NAME} COPY_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
endif()
