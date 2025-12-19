cmake_minimum_required(VERSION 3.8)

set(APP_NAME Sigeluguanshou)
project(${APP_NAME})

# 1. 设置引擎路径
set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)

include(CocosBuildSet)

# 2. 【关键修改】只添加引擎核心模块，避免冲突，并指定构建输出目录
add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${CMAKE_BINARY_DIR}/cocos)

# 3. 设置源文件
set(GAME_SRC
    main.cpp
    AppDelegate.cpp
    AppDelegate.h
    GameScene.cpp
    GameScene.h
    MapLayer.cpp
    MapLayer.h
    MenuScene.cpp
    MenuScene.h
    Player.cpp
    Player.h
)

# 4. 构建游戏
cocos_build_app(${APP_NAME}
    APP_SRC "${GAME_SRC}"
    DEPEND_COMMON_LIBS "cocos2d"
)

# 5. 确保头文件路径正确
target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})