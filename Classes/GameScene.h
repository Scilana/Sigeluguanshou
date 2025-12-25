#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include <string>
#include <vector>
#include "MapLayer.h"
#include "Player.h"
#include "FarmManager.h"
#include "FishingLayer.h"
#include "InventoryManager.h" // 包含 ItemType 定义
#include "InventoryUI.h"
#include "MarketState.h"
#include "MineScene.h"
#include "SaveManager.h"
#include "StorageChest.h"
#include "DialogueBox.h"
#include "Npc.h"

class MarketUI;
class WeatherManager;
class SkillTreeUI;

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
     * @brief 创建场景（新游戏）
     */
    static cocos2d::Scene* createScene();

    /**
     * @brief 创建场景（从存档加载）
     * @param loadFromSave 是否从存档加载
     */
    static cocos2d::Scene* createScene(bool loadFromSave);

    /**
     * @brief 初始化
     */
    virtual bool init() override;

    /**
     * @brief 初始化并加载存档
     * @param loadFromSave 是否从存档加载
     */
    bool init(bool loadFromSave);

    /**
     * @brief 更新函数
     * @param delta 时间增量
     */
    virtual void update(float delta) override;

    CREATE_FUNC(GameScene);

private:
    // ==========================================
    // 核心组件
    // ==========================================
    // ???

    MapLayer* mapLayer_;

    // ????

    FarmManager* farmManager_;

    // ??

    Player* player_;

    // ??????

    WeatherManager* weatherManager_;


    // ==========================================
    // 背包与系统
    // ==========================================
    InventoryManager* inventory_;
    InventoryUI* inventoryUI_;
    MarketState marketState_;
    MarketUI* marketUI_;
    SkillTreeUI* skillUI_;

    // ==========================================
    // UI层与元素
    // ==========================================
    cocos2d::Layer* uiLayer_;
    cocos2d::LayerColor* dayNightLayer_;
    cocos2d::Label* timeLabel_;
    cocos2d::Label* moneyLabel_;
    cocos2d::Label* positionLabel_;  // 显示玩家位置（调试用）
    cocos2d::Label* actionLabel_;    // 显示农场操作提示
    cocos2d::Label* itemLabel_;      // 显示当前物品
    cocos2d::LayerColor* toolbarUI_ = nullptr;
    std::vector<cocos2d::Sprite*> toolbarSlots_;
    std::vector<cocos2d::Sprite*> toolbarIcons_;
    std::vector<cocos2d::Label*> toolbarCounts_;
    std::vector<int> toolbarCountCache_;
    int toolbarSelectedCache_ = -1;

    // ==========================================
    // 初始化函数
    // ==========================================
    void initMap();
    void initFarm();
    void initPlayer();
    void initUI();
    void initCamera();
    void initControls();
    void initTrees(); // 初始化调试用树木标记
    void initWeather();
    void initNpcs();
    void initToolbarUI();
    void refreshToolbarUI();

    // ==========================================
    // 更新循环函数
    // ==========================================
    void updateCamera(); // 更新摄像机位置（跟随玩家）
    void updateUI();     // 更新UI显示
    void updateWeather();
    void updateDayNightLighting();

    // ==========================================
    // 控制与交互
    // ==========================================
    void backToMenu();
    void toggleInventory();
    void toggleSkillTree();
    void onInventoryClosed();
    void toggleMarket();
    void onMarketClosed();
    void enterMine();
    void enterHouse();

    /**
     * @brief 检查玩家是否在电梯附近
     */
    bool isPlayerNearElevator() const;
    bool isPlayerNearHouseDoor() const;

    // 电梯位置（农场地图上的坐标，需根据实际地图调整）
    // 假设在地图右上角附近 (例如 30*32, 20*32 处)
    // 根据之前的 mine_floor1.tmx，可能是在某个边缘。
    // 这里我们定义一个常量，并在 updateUI 中显示位置辅助调试
    const cocos2d::Vec2 ELEVATOR_POS = cocos2d::Vec2(800, 600);

    /**
     * @brief ESC键回调
     */


    /**
     * @brief 处理农田动作（J: till/plant/harvest，K: water）
     * @param waterOnly true=仅浇水，false=按顺序收获/种植/耕地
     */
    void handleFarmAction(bool waterOnly);

    /**
     * @brief 处理储物箱放置 (K键)
     */
    void handleChestPlacement();

    /**
     * @brief 打开储物箱界面
     */
    void openChestInventory(StorageChest* chest);

    /**
     * @brief 显示一次性的操作提示
     */
    void showActionMessage(const std::string& text, const cocos2d::Color3B& color);

    /**
     * @brief ESC键回调
     */
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    
    // 鼠标回调 (钓鱼用)
    void onMouseDown(cocos2d::Event* event);
    void onMouseUp(cocos2d::Event* event);

    // ==========================================
    // 钓鱼系统 (Fishing Logic)
    // ==========================================
    void startFishing();
    void updateFishingState(float delta);

    enum class FishingState { NONE, CHARGING, WAITING, BITING, REELING };
    FishingState fishingState_ = FishingState::NONE;
    float chargePower_ = 0.0f;
    float fishingTimer_ = 0.0f;
    float waitTimer_ = 0.0f;
    float biteTimer_ = 0.0f;
    bool isFishing_ = false;

    cocos2d::Sprite* chargeBarBg_ = nullptr;
    cocos2d::Sprite* chargeBarFg_ = nullptr;
    cocos2d::Sprite* exclamationMark_ = nullptr;
    
    // ==========================================
    // NPC System
    // ==========================================
    DialogueBox* dialogueBox_ = nullptr;
    std::vector<Npc*> npcs_;

    // ==========================================
    // 农场与工具栏操作
    // ==========================================


    // 快捷栏
    std::vector<ItemType> toolbarItems_;
    int selectedItemIndex_;
    void initToolbar();
    void selectItemByIndex(int idx);
    int getCropIdForItem(ItemType type) const;
    ItemType getItemTypeForCropId(int cropId) const;

    int lastWeatherDay_ = 0;

    // ==========================================
    // 砍树系统 (New Architecture from GameScene1)
    // ==========================================
    
    /**
     * @brief 砍树核心数据结构
     */
    struct TreeChopData
    {
        cocos2d::Vec2 tileCoord;         // 点击的瓦片坐标
        std::vector<cocos2d::Vec2> tiles;// 整棵树包含的所有瓦片
        cocos2d::Sprite* treeSprite;     // 替换后的树木精灵
        float chopTimer;                 // 计时器（用于超时重置）
        int chopCount;                   // 当前砍伐次数
        static const int CHOPS_NEEDED = 3; // 砍倒所需的次数
    };
    std::vector<TreeChopData> activeChops_; // 当前正在被砍的树
    std::vector<cocos2d::Vec2> choppedTrees_; // 已砍倒的树木位置（用于存档）

    // 调试用树木结构 (旧)
    struct Tree
    {
        cocos2d::Vec2 tileCoord;
        cocos2d::Node* node;
    };
    std::vector<Tree> trees_;
    int findTreeIndex(const cocos2d::Vec2& tile) const;

    /**
     * @brief 收集与起点相连的所有同类型碰撞组件（用于识别整棵树）
     */
    std::vector<cocos2d::Vec2> collectCollisionComponent(const cocos2d::Vec2& start) const;

    /**
     * @brief 在玩家周围寻找最近的树根碰撞体
     */
    bool findNearbyCollisionTile(const cocos2d::Vec2& centerTile, cocos2d::Vec2& outTile) const;

    /**
     * @brief 创建树木精灵（清除瓦片并在原位生成Sprite）
     */
    cocos2d::Sprite* createTreeSprite(const std::vector<cocos2d::Vec2>& tiles);

    /**
     * @brief 播放树木摇晃动画
     */
    void playTreeShakeAnimation(cocos2d::Sprite* treeSprite);

    /**
     * @brief 播放树木倒下动画
     */
    void playTreeFallAnimation(TreeChopData* chopData);

    /**
     * @brief 生成掉落物
     */
    void spawnItem(ItemType type, const cocos2d::Vec2& position, int count);

    /**
     * @brief 更新砍树状态（如超时重置）
     */
    void updateChopping(float delta);

    // ==========================================
    // 存档系统
    // ==========================================

    /**
     * @brief 保存游戏
     */
    void saveGame();

    /**
     * @brief 加载游戏
     */
    void loadGame();

    /**
     * @brief 收集游戏数据用于保存
     */
    SaveManager::SaveData collectSaveData();

    /**
     * @brief 应用加载的游戏数据
     */
    void applySaveData(const SaveManager::SaveData& data);
};

#endif // __GAME_SCENE_H__
