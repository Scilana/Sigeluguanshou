﻿#include "GameScene.h"
#include "MenuScene.h"
#include "SimpleAudioEngine.h"
#include "ui/CocosGUI.h"
#include "HouseScene.h"
#include "FarmManager.h"
#include "MarketUI.h"
#include "ElevatorUI.h"
#include <cmath>

USING_NS_CC;
using namespace CocosDenshion;

const float TILE_SIZE = 32.0f;

namespace
{
    const Vec2 kHouseDoorTile(20.0f, 15.0f);
    const float kHouseDoorRadius = 40.0f;
}

Scene* GameScene::createScene()
{
    return createScene(false);
}

Scene* GameScene::createScene(bool loadFromSave)
{
    GameScene* ret = new (std::nothrow) GameScene();
    if (ret && ret->init(loadFromSave))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GameScene::init()
{
    return init(false);
}

bool GameScene::init(bool loadFromSave)
{
    if (!Scene::init())
    {
        return false;
    }

    // 初始化核心管理器
    inventory_ = InventoryManager::getInstance(); // 单例
    // SaveManager 不需要 init

    // 初始化成员变量
    mapLayer_ = nullptr;
    farmManager_ = nullptr;
    player_ = nullptr;
    uiLayer_ = nullptr;
    inventoryUI_ = nullptr;
    selectedItemIndex_ = 0;
    fishingState_ = FishingState::NONE;
    isFishing_ = false;

    // 按顺序初始化组件
    initMap();

    // 初始化农场管理器
    farmManager_ = FarmManager::create(mapLayer_);
    this->addChild(farmManager_); // 添加到场景层

    initPlayer();
    initCamera();
    initUI();
    initControls();
    initToolbar();
    initTrees(); // 调试用树木标记

    // 如果是加载存档
    if (loadFromSave)
    {
        loadGame();
    }
    else
    {
        // 新游戏默认给予一些初始物品
        if (inventory_->getSlot(0).isEmpty())
        {
            inventory_->addItem(ItemType::Hoe, 1);
            inventory_->addItem(ItemType::WateringCan, 1);
            inventory_->addItem(ItemType::Axe, 1);
            inventory_->addItem(ItemType::Pickaxe, 1);
            inventory_->addItem(ItemType::Scythe, 1);
            inventory_->addItem(ItemType::SeedTurnip, 5);
        }
    }

    // 开启 Update
    this->scheduleUpdate();

    return true;
}



void GameScene::initMap()
{
    mapLayer_ = MapLayer::create("map/Farm.tmx");
    if (mapLayer_)
    {
        this->addChild(mapLayer_, -1);
    }
    else
    {
        CCLOG("Error loading map/Farm.tmx");
    }
}

void GameScene::initPlayer()
{
    player_ = Player::create();
    player_->setPosition(Vec2(400, 300)); // 默认位置
    player_->setMapLayer(mapLayer_);
    player_->enableKeyboardControl();
    this->addChild(player_, 10); // 玩家层级较高
}



void GameScene::initCamera()
{
    // 使用默认摄像机
    auto camera = this->getDefaultCamera();
    updateCamera(); // 初始位置更新
}



void GameScene::initUI()
{
    // 创建UI层，随摄像机移动（或使用ScreenSpaceCamera，但这里简单模拟）
    uiLayer_ = Layer::create();
    this->addChild(uiLayer_, 100);

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 时间显示
    timeLabel_ = Label::createWithSystemFont("Day 1  06:00 AM", "Arial", 24);
    timeLabel_->setPosition(Vec2(visibleSize.width - 20, visibleSize.height - 20));
    timeLabel_->setAnchorPoint(Vec2(1, 1));
    uiLayer_->addChild(timeLabel_);

    // 金钱显示
    moneyLabel_ = Label::createWithSystemFont("Gold: 0", "Arial", 24);
    moneyLabel_->setPosition(Vec2(visibleSize.width - 20, visibleSize.height - 50));
    moneyLabel_->setAnchorPoint(Vec2(1, 1));
    moneyLabel_->setColor(Color3B::YELLOW);
    uiLayer_->addChild(moneyLabel_);

    // 物品栏显示
    itemLabel_ = Label::createWithSystemFont("Hands: Empty", "Arial", 24);
    itemLabel_->setPosition(Vec2(20, 20)); // 左下角
    itemLabel_->setAnchorPoint(Vec2(0, 0));
    uiLayer_->addChild(itemLabel_);

    // 操作提示
    actionLabel_ = Label::createWithSystemFont("", "Arial", 32);
    actionLabel_->setAnchorPoint(Vec2(0.5f, 0.5f));
    actionLabel_->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2 + 50
    ));
    actionLabel_->setOpacity(0);
    uiLayer_->addChild(actionLabel_);

    // 位置调试
    positionLabel_ = Label::createWithSystemFont("Pos: (0,0)", "Arial", 16);
    positionLabel_->setPosition(Vec2(20, visibleSize.height - 20));
    positionLabel_->setAnchorPoint(Vec2(0, 1));
    uiLayer_->addChild(positionLabel_);
}



void GameScene::initControls()
{
    // 键盘监听
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(GameScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    CCLOG("Controls initialized");

}



void GameScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)

{

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_ESCAPE:
        backToMenu();
        break;
    case EventKeyboard::KeyCode::KEY_B:
        toggleInventory();
        break;
    case EventKeyboard::KeyCode::KEY_P:
        toggleMarket();
        break;
    case EventKeyboard::KeyCode::KEY_L:
        // 直接进入矿洞（使用 Mines/1.tmx）
        enterMine();
        break;
    case EventKeyboard::KeyCode::KEY_X:
        // 保存游戏
        saveGame();
        break;
    case EventKeyboard::KeyCode::KEY_M:
        if (isPlayerNearElevator())
        {
            enterMine();
        }
        else
        {
            showActionMessage("Elevator is too far!", Color3B::RED);
            // 调试显示位置
            CCLOG("Player at (%.1f, %.1f), Elevator at (%.1f, %.1f)",
                player_->getPosition().x, player_->getPosition().y,
                ELEVATOR_POS.x, ELEVATOR_POS.y);
        }
        break;
    case EventKeyboard::KeyCode::KEY_ENTER:
    case EventKeyboard::KeyCode::KEY_KP_ENTER:
        enterHouse();
        break;
    case EventKeyboard::KeyCode::KEY_J:
        handleFarmAction(false);  // till / plant / harvest
        break;
    case EventKeyboard::KeyCode::KEY_K:
        handleFarmAction(true);   // water
        break;
    case EventKeyboard::KeyCode::KEY_1:
        selectItemByIndex(0);
        break;
    case EventKeyboard::KeyCode::KEY_2:
        selectItemByIndex(1);
        break;
    case EventKeyboard::KeyCode::KEY_3:
        selectItemByIndex(2);
        break;
    case EventKeyboard::KeyCode::KEY_4:
        selectItemByIndex(3);
        break;
    case EventKeyboard::KeyCode::KEY_5:
        selectItemByIndex(4);
        break;
    case EventKeyboard::KeyCode::KEY_6:
        selectItemByIndex(5);
        break;
    case EventKeyboard::KeyCode::KEY_7:
        selectItemByIndex(6);
        break;
    case EventKeyboard::KeyCode::KEY_8:
        selectItemByIndex(7);
        break;
    case EventKeyboard::KeyCode::KEY_9:
        selectItemByIndex(8);
        break;
    case EventKeyboard::KeyCode::KEY_0:
        selectItemByIndex(9);
        break;
    case EventKeyboard::KeyCode::KEY_MINUS: // Allow selecting item 10
        selectItemByIndex(10);
        break;
    case EventKeyboard::KeyCode::KEY_EQUAL: // Allow selecting item 11
        selectItemByIndex(11);
        break;
    default:
        break;
    }

}

// ========== 更新函数 ==========

void GameScene::update(float delta)
{
    Scene::update(delta);

    // 更新各组件
    if (farmManager_) farmManager_->update(delta);
    updateCamera();
    updateUI();
    updateChopping(delta);
    if (isFishing_) updateFishingState(delta);

    // 时间流逝逻辑可在此添加 (WeatherManager handles time?)
}

void GameScene::updateCamera()
{
    if (!player_) return;
    
    auto camera = this->getDefaultCamera();
    Vec2 playerPos = player_->getPosition();
    Vec3 camPos = camera->getPosition3D();
    
    // 简单平滑跟随
    Vec3 targetPos(playerPos.x, playerPos.y, camPos.z);
    Vec3 newPos = camPos + (targetPos - camPos) * 0.1f;
    
    // 边界限制
    if (mapLayer_)
    {
         Size mapSize = mapLayer_->getMapSize();
         Size visibleSize = Director::getInstance()->getVisibleSize();
         
         float minX = visibleSize.width/2;
         float maxX = mapSize.width - visibleSize.width/2;
         float minY = visibleSize.height/2;
         float maxY = mapSize.height - visibleSize.height/2;

         if (mapSize.width < visibleSize.width) minX = maxX = mapSize.width/2;
         if (mapSize.height < visibleSize.height) minY = maxY = mapSize.height/2;

         newPos.x = std::max(minX, std::min(maxX, newPos.x));
         newPos.y = std::max(minY, std::min(maxY, newPos.y));
    }
    
    camera->setPosition3D(newPos);

    // UI层跟随摄像机以保持屏幕位置固定
    if (uiLayer_)
    {
        uiLayer_->setPosition(Vec2(
            newPos.x - Director::getInstance()->getVisibleSize().width / 2,
            newPos.y - Director::getInstance()->getVisibleSize().height / 2
        ));
    }
}

void GameScene::updateUI()
{
    if (inventory_)
    {
        moneyLabel_->setString(StringUtils::format("Gold: %d", inventory_->getMoney()));
        
        // 更新手持物品显示
        ItemType handItem = inventory_->getSlot(selectedItemIndex_).type;
        std::string name = InventoryManager::getItemName(handItem);
        if (handItem == ItemType::None) name = "Empty";
        itemLabel_->setString(StringUtils::format("[%d] %s", selectedItemIndex_ + 1, name.c_str()));
    }

    if (player_)
    {
        Vec2 pos = player_->getPosition();
        positionLabel_->setString(StringUtils::format("Pos: (%.0f, %.0f)", pos.x, pos.y));
    }
}

// =====================================
// 输入处理
// =====================================

void GameScene::selectItemByIndex(int idx)
{
    if (idx >= 0 && idx < 10)
    {
        selectedItemIndex_ = idx;
        if (inventory_)
        {
            inventory_->setSelectedSlotIndex(idx);
        }
    }
}

void GameScene::toggleInventory()
{
    if (inventoryUI_)
    {
        inventoryUI_->close(); // 这会触发回调 onInventoryClosed
        // 但如果 close 是动画，回调会延迟。
        // 为防止重复操作，这里不设置为nullptr，由回调设。
    }
    else
    {
        inventoryUI_ = InventoryUI::create(inventory_);
        if (inventoryUI_)
        {
            inventoryUI_->setCloseCallback(CC_CALLBACK_0(GameScene::onInventoryClosed, this));
            // 添加到UI层顶层
            if (uiLayer_) uiLayer_->addChild(inventoryUI_, 999);
            inventoryUI_->show();
            inventoryUI_->setPosition(Vec2::ZERO); // 相对UI层
        }
    }
}

void GameScene::backToMenu()
{
    // 保存并返回
    saveGame();
    Director::getInstance()->replaceScene(MenuScene::createScene());
}

// =====================================
// 农场与工具逻辑
// =====================================

void GameScene::handleFarmAction(bool waterOnly)
{
    if (!player_ || !farmManager_ || !inventory_ || !mapLayer_)
        return;

    ItemType currentItem = inventory_->getSlot(selectedItemIndex_).type;
    
    // 获取交互的目标瓦片（玩家前方）
    Vec2 playerPos = player_->getPosition();
    Vec2 facing = player_->getFacingDirection();
    Vec2 targetPos = playerPos + facing * 32.0f; // 前方一格
    Vec2 tileCoord = mapLayer_->positionToTileCoord(targetPos);

    if (waterOnly)
    {
        if (currentItem == ItemType::WateringCan)
        {
            auto result = farmManager_->waterTile(tileCoord);
            if (!result.message.empty())
                showActionMessage(result.message, result.success ? Color3B::BLUE : Color3B::RED);
        }
        return;
    }

    FarmManager::ActionResult result{ false, "", -1 };

    if (currentItem == ItemType::Hoe)
    {
        result = farmManager_->tillTile(tileCoord);
    }
    else
    {
        int cropId = getCropIdForItem(currentItem);
        if (cropId >= 0)
        {
            result = farmManager_->plantSeed(tileCoord, cropId);
            if (result.success)
            {
                inventory_->removeItem(currentItem, 1);
            }
        }
        else
        {
            result = farmManager_->harvestTile(tileCoord);
            if (result.success)
            {
                ItemType cropItem = getItemTypeForCropId(result.cropId);
                if (cropItem != ItemType::None)
                {
                    inventory_->addItem(cropItem, 1);
                }
            }
        }
    }

    if (!result.message.empty())
    {
        showActionMessage(result.message, result.success ? Color3B::GREEN : Color3B::RED);
    }
}

void GameScene::onInventoryClosed()
{
    inventoryUI_ = nullptr;
    CCLOG("Inventory closed");
}

void GameScene::toggleMarket()
{
    if (!inventory_)
    {
        CCLOG("ERROR: Inventory not initialized!");
        return;
    }

    if (marketUI_)
    {
        marketUI_->close();
        return;
    }

    marketUI_ = MarketUI::create(inventory_, &marketState_, farmManager_);
    if (!marketUI_)
    {
        CCLOG("ERROR: Failed to create MarketUI!");
        return;
    }

    marketUI_->setCloseCallback([this]() {
        onMarketClosed();
    });

    this->addChild(marketUI_, 2200);
    marketUI_->setGlobalZOrder(2200);

    auto camera = this->getDefaultCamera();
    if (camera)
    {
        Vec3 cameraPos = camera->getPosition3D();
        auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
        Vec2 uiPos = Vec2(
            cameraPos.x - visibleSize.width / 2,
            cameraPos.y - visibleSize.height / 2
        );
        marketUI_->setPosition(uiPos);
    }

    marketUI_->show();
    CCLOG("Market opened");
}

void GameScene::onMarketClosed()
{
    marketUI_ = nullptr;
    CCLOG("Market closed");
}

void GameScene::enterHouse()
{
    if (!isPlayerNearHouseDoor())
    {
        showActionMessage("Door is too far!", Color3B::RED);
        return;
    }

    auto houseScene = HouseScene::createScene();
    if (!houseScene)
    {
        CCLOG("ERROR: Failed to create house scene!");
        return;
    }

    auto transition = TransitionFade::create(0.4f, houseScene);
    Director::getInstance()->pushScene(transition);
}

void GameScene::enterMine()
{
    CCLOG("Opening elevator UI...");

    // 创建电梯楼层选择UI
    auto elevatorUI = ElevatorUI::create();
    if (!elevatorUI)
    {
        CCLOG("ERROR: Failed to create ElevatorUI!");
        return;
    }

    // 设置楼层选择回调
    elevatorUI->setFloorSelectCallback([this](int floor) {
        CCLOG("Floor %d selected, entering mine...", floor);

        if (floor == 0)
        {
            // 楼层0是农场（地面），不需要切换场景
            showActionMessage("Already on the farm!", Color3B(200, 200, 200));
            return;
        }

        // 进入矿洞前自动保存游戏
        CCLOG("Auto-saving before entering mine...");
        SaveManager::SaveData data = collectSaveData();
        if (SaveManager::getInstance()->saveGame(data))
        {
            CCLOG("✓ Auto-save successful!");
        }
        else
        {
            CCLOG("✗ Auto-save failed!");
        }

        // 创建矿洞场景，传入背包实例和选择的楼层
        auto mineScene = MineScene::createScene(inventory_, floor);
        if (mineScene)
        {
            auto transition = TransitionFade::create(0.5f, mineScene);
            Director::getInstance()->replaceScene(transition);
        }
        else
        {
            CCLOG("ERROR: Failed to create mine scene for floor %d!", floor);
        }
    });

    // 添加到UI层（uiLayer_ 会随摄像机移动，所以这里只需添加到它上面）
    if (uiLayer_)
    {
        elevatorUI->setPosition(Vec2::ZERO);
        uiLayer_->addChild(elevatorUI, 2000); 
    }
    else
    {
        // 回退逻辑
        this->addChild(elevatorUI, 2500);
        auto camera = this->getDefaultCamera();
        if (camera)
        {
            Vec3 cameraPos = camera->getPosition3D();
            auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
            Vec2 uiPos = Vec2(
                cameraPos.x - visibleSize.width / 2,
                cameraPos.y - visibleSize.height / 2
            );
            elevatorUI->setPosition(uiPos);
        }
    }

    elevatorUI->show();
}

bool GameScene::isPlayerNearElevator() const
{
    if (!player_) return false;

    // 检查距离
    float distance = player_->getPosition().distance(ELEVATOR_POS);
    // 允许100像素误差
    return distance < 100.0f;
}

bool GameScene::isPlayerNearHouseDoor() const
{
    if (!player_ || !mapLayer_)
        return false;

    Vec2 doorPos = mapLayer_->tileCoordToPosition(kHouseDoorTile);
    return player_->getPosition().distance(doorPos) <= kHouseDoorRadius;
}

// ==========================================
// 存档系统实现
// ==========================================

// ... 鐎圭偟骞 Save/Load Stubs
void GameScene::saveGame()
{
    if (SaveManager::getInstance()->saveGame(collectSaveData()))
        showActionMessage("Game Saved!", Color3B::GREEN);
}

void GameScene::loadGame()
{
    SaveManager::SaveData data;
    if (SaveManager::getInstance()->loadGame(data))
    {
        applySaveData(data);
        showActionMessage("Game Loaded!", Color3B::GREEN);
    }
}

SaveManager::SaveData GameScene::collectSaveData()
{
    SaveManager::SaveData data;
    if (player_) {
        data.playerPosition = player_->getPosition();
    }
    // TODO: collect farm data, inventory data etc.
    return data;
}

void GameScene::applySaveData(const SaveManager::SaveData& data)
{
    if (player_) {
        player_->setPosition(data.playerPosition);
    }
    if (inventory_) {
        // InventoryManager should have its own load/save logic typically, or controlled here
        // inventory_->setData(data.inventoryData);
    }
}

// ... 实现 Fishing Stubs
void GameScene::onMouseDown(Event* event) { }
void GameScene::onMouseUp(Event* event) { }
void GameScene::startFishing() { }
void GameScene::updateFishingState(float delta) { }

// ... 实现 Tree Chopping Stubs
std::vector<cocos2d::Vec2> GameScene::collectCollisionComponent(const cocos2d::Vec2& start) const { return {start}; }
bool GameScene::findNearbyCollisionTile(const cocos2d::Vec2& centerTile, cocos2d::Vec2& outTile) const { 
    // 简单Mock
    if (mapLayer_ && !mapLayer_->isWalkable(centerTile)) {
        outTile = centerTile;
        return true;
    }
    return false; 
}
cocos2d::Sprite* GameScene::createTreeSprite(const std::vector<cocos2d::Vec2>& tiles) { 
    // Mock tree
    if (!mapLayer_) return nullptr;
    auto sp = Sprite::create("env/tree_mock.png"); 
    if (!sp) sp = Sprite::create(); // fallback
    sp->setPosition(mapLayer_->tileCoordToPosition(tiles[0])); 
    if (mapLayer_) mapLayer_->addChild(sp, 5);
    return sp; 
}
void GameScene::playTreeShakeAnimation(cocos2d::Sprite* treeSprite) {
    if (treeSprite) treeSprite->runAction(Sequence::create(RotateBy::create(0.1, 5), RotateBy::create(0.1, -10), RotateBy::create(0.1, 5), nullptr));
}
void GameScene::playTreeFallAnimation(TreeChopData* chopData) {
    if (chopData && chopData->treeSprite) {
        chopData->treeSprite->runAction(Sequence::create(RotateBy::create(0.5, 90), FadeOut::create(0.5), RemoveSelf::create(), nullptr));
        chopData->chopCount = -1; // Mark for removal
    }
}
void GameScene::spawnItem(ItemType type, const cocos2d::Vec2& position, int count) {
    // Mock spawn
}
void GameScene::updateChopping(float delta) {
    // remove finished chops
}

void GameScene::initFarm()
{
    // Farm initialization logic if needed, currently handled in init()
}

void GameScene::initToolbar()
{
    toolbarItems_.clear();
    selectedItemIndex_ = 0;
    if (inventory_)
    {
        inventory_->setSelectedSlotIndex(selectedItemIndex_);
    }
}

void GameScene::initTrees()
{
    trees_.clear();
    activeChops_.clear();
    choppedTrees_.clear();
}

int GameScene::findTreeIndex(const cocos2d::Vec2& tile) const
{
    for (size_t i = 0; i < trees_.size(); ++i)
    {
        if (trees_[i].tileCoord == tile)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int GameScene::getCropIdForItem(ItemType type) const
{
    switch (type)
    {
    case ItemType::SeedTurnip:
        return 0;
    case ItemType::SeedPotato:
        return 1;
    case ItemType::SeedCorn:
        return 2;
    case ItemType::SeedTomato:
        return 3;
    case ItemType::SeedPumpkin:
        return 4;
    case ItemType::SeedBlueberry:
        return 5;
    default:
        return -1;
    }
}

void GameScene::showActionMessage(const std::string& text, const cocos2d::Color3B& color)
{
    if (!actionLabel_)
        return;

    actionLabel_->stopAllActions();
    actionLabel_->setString(text);
    actionLabel_->setColor(color);
    actionLabel_->setOpacity(255);
    auto seq = Sequence::create(DelayTime::create(1.0f), FadeOut::create(0.5f), nullptr);
    actionLabel_->runAction(seq);
}

ItemType GameScene::getItemTypeForCropId(int cropId) const
{
    switch (cropId)
    {
    case 0:
        return ItemType::Turnip;
    case 1:
        return ItemType::Potato;
    case 2:
        return ItemType::Corn;
    case 3:
        return ItemType::Tomato;
    case 4:
        return ItemType::Pumpkin;
    case 5:
        return ItemType::Blueberry;
    default:
        return ItemType::None;
    }
}
