#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "MapLayer.h"
#include "Player.h"

/**
 * @brief 游戏场景类（总控制）
 *
 * 职责：
 * - 组合地图层、玩家、摄像机
 * - 场景级别的管理和协调
 * - 摄像机跟随玩家
 * - 不负责具体的绘图工作
 */
class GameScene : public cocos2d::Scene
{
public:
    /**
     * @brief 创建场景
     */
    static cocos2d::Scene* createScene();

    /**
     * @brief 初始化
     */
    virtual bool init() override;

    /**
     * @brief 更新函数
     * @param delta 时间增量
     */
    virtual void update(float delta) override;

    CREATE_FUNC(GameScene);

private:
    // 地图层
    MapLayer* mapLayer_;

    // 玩家
    Player* player_;

    // UI层（用于显示HUD等）
    cocos2d::Layer* uiLayer_;

    // UI元素
    cocos2d::Label* timeLabel_;
    cocos2d::Label* moneyLabel_;
    cocos2d::Label* positionLabel_;  // 显示玩家位置（调试用）

    /**
     * @brief 初始化地图
     */
    void initMap();

    /**
     * @brief 初始化玩家
     */
    void initPlayer();

    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 初始化摄像机
     */
    void initCamera();

    /**
     * @brief 初始化控制
     */
    void initControls();

    /**
     * @brief 更新摄像机位置（跟随玩家）
     */
    void updateCamera();

    /**
     * @brief 更新UI显示
     */
    void updateUI();

    /**
     * @brief 返回菜单
     */
    void backToMenu();

    /**
     * @brief ESC键回调
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
};

#endif // __GAME_SCENE_H__