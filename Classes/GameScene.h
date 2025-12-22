#ifndef __GAME_SCENE_H__

#define __GAME_SCENE_H__

#include "cocos2d.h"
#include <string>
#include <vector>
#include "MapLayer.h"
#include "Player.h"
#include "FarmManager.h"
#include "FishingLayer.h"

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

    // 农场管理
    FarmManager* farmManager_;

    // 玩家
    Player* player_;

    // UI层（用于显示HUD等）

    cocos2d::Layer* uiLayer_;

    // UI元素

    cocos2d::Label* timeLabel_;
    cocos2d::Label* moneyLabel_;
    cocos2d::Label* positionLabel_;  // 显示玩家位置（调试用）
    cocos2d::Label* actionLabel_;    // 显示农场操作提示
    cocos2d::Label* itemLabel_;      // 显示当前物品

    /**
     * @brief 初始化地图
     */
    void initMap();

    /**
     * @brief 初始化农田管理
     */
    void initFarm();

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


    /**
     * @brief 处理农田动作（J: till/plant/harvest，K: water）
     * @param waterOnly true=仅浇水，false=按顺序收获/种植/耕地
     */
    void handleFarmAction(bool waterOnly);

    /**
     * @brief 显示一次性的操作提示
     */
    void showActionMessage(const std::string& text, const cocos2d::Color3B& color);

    /**
     * @brief ESC键回调
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    // 【Fishing Restored】鼠标事件
    void onMouseDown(cocos2d::Event* event);
    void onMouseUp(cocos2d::Event* event);

    // Fishing Logic
    void startFishing();
    void updateFishingState(float delta);

    enum class FishingState { NONE, CHARGING, WAITING, BITING, REELING };
    FishingState fishingState_ = FishingState::NONE;
    float chargePower_ = 0.0f;
    float fishingTimer_ = 0.0f; // For generic timer use if needed
    float waitTimer_ = 0.0f;
    float biteTimer_ = 0.0f;
    bool isFishing_ = false;

    cocos2d::Sprite* chargeBarBg_ = nullptr;
    cocos2d::Sprite* chargeBarFg_ = nullptr;
    cocos2d::Sprite* exclamationMark_ = nullptr;

    /**
     * @brief 处理农田动作（J: till/plant/harvest，K: water）
     * @param waterOnly true=仅浇水，false=按顺序收获/种植/耕地
     */
// ... existing code ...
    // 物品栏
    enum class ItemType
    {
        Hoe,
        WateringCan,
        Scythe,
        Axe,
        Pickaxe,
        FishingRod, // Added Fishing Rod
        SeedTurnip,
// ... existing code ...
        SeedPotato,
        SeedCorn,
        SeedTomato,
        SeedPumpkin,
        SeedBlueberry,
        Wood
    };

    std::vector<ItemType> toolbarItems_;
    int selectedItemIndex_;
    void initToolbar();
    void selectItemByIndex(int idx);
    std::string getItemName(ItemType type) const;
    int getCropIdForItem(ItemType type) const;
    std::string getItemNameChinese(ItemType type) const;
    std::vector<cocos2d::Vec2> collectCollisionComponent(const cocos2d::Vec2& start) const;
    bool findNearbyCollisionTile(const cocos2d::Vec2& centerTile, cocos2d::Vec2& outTile) const;

    // 树木
    struct Tree
    {
        cocos2d::Vec2 tileCoord;
        cocos2d::Node* node;
    };
    std::vector<Tree> trees_;

    struct PendingChop
    {
        std::vector<cocos2d::Vec2> tiles;
        float timer;
        int fillGid;
    };
    std::vector<PendingChop> pendingChops_;
    void initTrees();
    int findTreeIndex(const cocos2d::Vec2& tile) const;

    // 砍树数据结构（核心）
    struct TreeChopData
    {
        cocos2d::Vec2 tileCoord;
        std::vector<cocos2d::Vec2> tiles;
        cocos2d::Sprite* treeSprite;
        float chopTimer;
        int chopCount;
        static const int CHOPS_NEEDED = 3;
    };
    std::vector<TreeChopData> activeChops_;

    // 砍树相关函数
    cocos2d::Sprite* createTreeSprite(const std::vector<cocos2d::Vec2>& tiles);
    void playTreeShakeAnimation(cocos2d::Sprite* treeSprite);
    void playTreeFallAnimation(TreeChopData* chopData);
    void spawnItem(ItemType type, const cocos2d::Vec2& position, int count);


    void updateChopping(float delta);
};

#endif // __GAME_SCENE_H__


