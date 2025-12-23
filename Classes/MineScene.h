#ifndef __MINE_SCENE_H__
#define __MINE_SCENE_H__

#include "cocos2d.h"
#include <string>
#include <vector>
#include "MineLayer.h"
#include "Player.h"
#include "MiningManager.h"
#include "InventoryManager.h"

/**
 * @brief 矿洞场景类
 *
 * 职责：
 * - 管理矿洞地图层
 * - 控制玩家在矿洞中的行为
 * - 处理挖矿逻辑
 * - 摄像机跟随
 */
class MineScene : public cocos2d::Scene
{
public:
    /**
     * @brief 创建矿洞场景
     * @param inventory 背包管理器引用（从主场景传递）
     * @param currentFloor 当前矿洞层数
     */
    static MineScene* createScene(InventoryManager* inventory, int currentFloor = 1);

    /**
     * @brief 初始化
     * @param inventory 背包管理器引用
     * @param currentFloor 当前矿洞层数
     */
    virtual bool init(InventoryManager* inventory, int currentFloor);

    /**
     * @brief 每帧更新
     */
    virtual void update(float delta) override;

private:
    // 地图层
    MineLayer* mineLayer_;

    // 挖矿管理器
    MiningManager* miningManager_;

    // 玩家
    Player* player_;

    // 背包系统（共享引用）
    InventoryManager* inventory_;

    // UI层
    cocos2d::Layer* uiLayer_;

    // UI元素
    cocos2d::Label* floorLabel_;      // 矿洞层数显示
    cocos2d::Label* healthLabel_;     // 体力显示
    cocos2d::Label* positionLabel_;   // 位置显示（调试）
    cocos2d::Label* actionLabel_;     // 操作提示
    cocos2d::Label* itemLabel_;       // 当前工具

    // 当前矿洞层数
    int currentFloor_;

    /**
     * @brief 初始化地图
     */
    void initMap();

    /**
     * @brief 初始化挖矿管理器
     */
    void initMining();

    /**
     * @brief 初始化玩家
     */
    void initPlayer();

    /**
     * @brief 初始化摄像机
     */
    void initCamera();

    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 初始化控制
     */
    void initControls();

    /**
     * @brief 更新摄像机（跟随玩家）
     */
    void updateCamera();

    /**
     * @brief 更新UI显示
     */
    void updateUI();

    /**
     * @brief 处理挖矿动作
     */
    void handleMiningAction();

    /**
     * @brief 显示操作提示
     */
    void showActionMessage(const std::string& text, const cocos2d::Color3B& color);

    /**
     * @brief 键盘事件
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    /**
     * @brief 返回地面（农场）
     */
    void backToFarm();

    /**
     * @brief 前往下一层
     */
    void goToNextFloor();

    /**
     * @brief 检查是否在楼梯位置
     */
    bool isPlayerOnStairs() const;
};

#endif // __MINE_SCENE_H__
