#ifndef __MINE_SCENE_H__
#define __MINE_SCENE_H__

#include "cocos2d.h"
#include <string>
#include <vector>
#include <map>

#include "InventoryManager.h"

// Forward declarations
class MineLayer;
class Player;
class InventoryUI;
class MiningManager;
class Monster;
class TreasureChest;
class Weapon;

class Slime;
class Zombie;

/**
 * @brief 矿洞场景类（简化版）
 *
 * 职责：
 * - 管理矿洞地图层
 * - 控制玩家在矿洞中的行为
 * - 处理挖矿逻辑
 * - 怪物和宝箱管理
 * - 摄像机跟随
 * - 场景切换
 */
class MineScene : public cocos2d::Scene
{
public:
    /**
     * @brief 创建矿洞场景
     * @param inventory 背包管理器引用（从主场景传递）
     * @param currentFloor 当前矿洞层数
     */
    static MineScene* createScene(InventoryManager* inventory, int currentFloor = 1, int dayCount = 1);

    /**
     * @brief 初始化
     * @param inventory 背包管理器引用
     * @param currentFloor 当前矿洞层数
     */
    virtual bool init(InventoryManager* inventory, int currentFloor, int dayCount);

    /**
     * @brief 每帧更新
     */
    virtual void update(float delta) override;

private:
    // 地图层
    MineLayer* mineLayer_;

    // 玩家
    Player* player_;

    // 背包系统（共享引用）
    InventoryManager* inventory_;
    InventoryUI* inventoryUI_;
    MiningManager* miningManager_;
    
    // 物品栏（使用 InventoryManager 中定义的 ItemType）
    std::vector<ItemType> toolbarItems_;
    int selectedItemIndex_;
    void initToolbar();
    void selectItemByIndex(int idx);
    
    // 打开/关闭背包
    void toggleInventory();
    void onInventoryClosed();

    // UI层
    cocos2d::Layer* uiLayer_;

    // UI元素
    cocos2d::Label* infoLabel_;                      // 物品信息显示

    int selectedSlotIndex_;                          // 当前选中的槽位索引
    void updateSelection();                          // 更新选中状态显示
    cocos2d::Label* floorLabel_;      // 矿洞层数显示
    cocos2d::Label* positionLabel_;   // 位置显示（调试）
    cocos2d::Label* itemLabel_;       // current tool label
    cocos2d::Label* actionLabel_;     // action hint label
    cocos2d::Label* healthLabel_;     // health label

    // 当前矿洞层数
    int currentFloor_;
    int dayCount_;

    // ========== 怪物系统 ==========
    std::vector<Monster*> monsters_;
    float monsterSpawnTimer_;

    // ========== 宝箱系统 ==========
    std::vector<TreasureChest*> chests_;

    // ========== 武器/攻击系统 ==========
    ItemType currentWeapon_;
    float attackCooldown_;
    float currentAttackCooldown_;

    // Wishing Well
    cocos2d::Node* wishingWell_;

    /**
     * @brief 初始化地图
     */
    void initMap();

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
     * @brief 初始化怪物
     */
    void initMonsters();

    /**
     * @brief 初始化宝箱
     */
    void initChests();

    /**
     * @brief 更新摄像机（跟随玩家）
     */
    void updateCamera();

    /**
     * @brief 更新UI显示
     */
    void updateUI();

    /**
     * @brief 更新怪物
     */
    void updateMonsters(float delta);

    /**
     * @brief 处理挖矿动作
     */
    void handleMiningAction();
    void executeMining(const cocos2d::Vec2& tileCoord);

    /**
     * @brief 处理攻击动作
     */
    void handleAttackAction();

    /**
     * @brief 处理宝箱交互
     */
    void handleChestInteraction();

    /**
     * @brief 初始化许愿池
     */
    void initWishingWell();

    /**
     * @brief 处理许愿池交互
     */
    void handleWishAction();

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
     * @brief 前往上一层
     */
    void goToPreviousFloor();

    /**
     * @brief 前往下一层
     */
    void goToNextFloor();

    /**
     * @brief 检查是否在楼梯位置
     */
    bool isPlayerOnStairs() const;

    /**
     * @brief 生成怪物（根据楼层）
     */
    void spawnMonster();

    /**
     * @brief 获取怪物生成概率
     */
    float getMonsterSpawnChance() const;

    /**
     * @brief 获取宝箱生成概率
     */
    float getChestSpawnChance() const;

    /**
     * @brief 获取随机可行走位置
     */
    cocos2d::Vec2 getRandomWalkablePosition() const;

    // 静态持久化数据：记录每一层宝箱最后一次被开启的周数
    // floor -> week
    static std::map<int, int> openedChestsPerWeek_;
};

#endif // __MINE_SCENE_H__
