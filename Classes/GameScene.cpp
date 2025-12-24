#include "GameScene.h"
#include "MenuScene.h"
#include "HouseScene.h"
#include "FarmManager.h"
#include "MarketUI.h"
#include "ElevatorUI.h"
#include "MarketUI.h"
#include "WeatherManager.h"
#include <cmath>
#include <queue>
#include <unordered_set>

USING_NS_CC;

namespace
{
    const Vec2 kHouseDoorTile(20.0f, 15.0f);
    const float kHouseDoorRadius = 40.0f;
}

Scene* GameScene::createScene()

{

    return GameScene::create();

}

bool GameScene::init()

{

    if (!Scene::init())

        return false;

    CCLOG("========================================");

    CCLOG("Initializing Game Scene (New Architecture)");

    CCLOG("========================================");

    // 初始化各个组件
    mapLayer_ = nullptr;
    farmManager_ = nullptr;
    player_ = nullptr;
    weatherManager_ = nullptr;
    uiLayer_ = nullptr;
    inventory_ = nullptr;
    inventoryUI_ = nullptr;
    marketUI_ = nullptr;

    initMap();
    initFarm();
    initTrees();
    initPlayer();
    initCamera();
    initUI();
    initControls();

    // 初始化背包系统
    inventory_ = InventoryManager::getInstance();
    if (inventory_)
    {
        // 不再添加到场景，避免随场景销毁
        // this->addChild(inventory_, 0);
        CCLOG("InventoryManager retrieved from singleton");
    }
    marketState_.init();
    initWeather();

    // --- 【Fishing Inputs】 ---
    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseDown = CC_CALLBACK_1(GameScene::onMouseDown, this);
    auto mouseUpListener = EventListenerMouse::create();
    mouseUpListener->onMouseUp = CC_CALLBACK_1(GameScene::onMouseUp, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseUpListener, this);

    // 启动更新
    this->scheduleUpdate();

    CCLOG("Game Scene initialized successfully!");

    return true;

}

// ========== 初始化地图 ==========

void GameScene::initMap()

{

    CCLOG("Initializing map...");

    // 创建地图层

    mapLayer_ = MapLayer::create("map/farm.tmx");

    if (mapLayer_)

    {

        this->addChild(mapLayer_, 0);

        CCLOG("Map layer added to scene");

    }

    else

    {

        CCLOG("ERROR: Failed to create map layer!");

        CCLOG("Please ensure farm.tmx exists in Resources/map/");

    }

}

void GameScene::initFarm()

{

    if (!mapLayer_)
    {

        CCLOG("No map layer, skip farm manager");

        return;

    }

    farmManager_ = FarmManager::create(mapLayer_);

    if (farmManager_)
    {

        mapLayer_->addChild(farmManager_, 5);

        CCLOG("FarmManager added on top of MapLayer");

    }

    else
    {

        CCLOG("Failed to create FarmManager");

    }

}

// ========== 初始化玩家 ==========

void GameScene::initPlayer()

{

    CCLOG("Initializing player...");

    // 创建玩家

    player_ = Player::create();

    if (player_)
    {
        if (mapLayer_)
        {
            Vec2 preferredPos(400.0f, 300.0f);
            if (mapLayer_->isWalkable(preferredPos))
            {
                player_->setPosition(preferredPos);
            }
            else
            {
                Size mapSize = mapLayer_->getMapSize();
                Vec2 centerPos = Vec2(mapSize.width / 2, mapSize.height / 2);

                bool centerWalkable = mapLayer_->isWalkable(centerPos);
                if (!centerWalkable)
                {
                    Vec2 safePos = centerPos;
                    bool foundSafe = false;

                    for (int radius = 1; radius < 10 && !foundSafe; radius++)
                    {
                        for (int dx = -radius; dx <= radius && !foundSafe; dx++)
                        {
                            for (int dy = -radius; dy <= radius && !foundSafe; dy++)
                            {
                                Vec2 testPos = centerPos + Vec2(dx * 32, dy * 32);
                                if (mapLayer_->isWalkable(testPos))
                                {
                                    safePos = testPos;
                                    foundSafe = true;
                                }
                            }
                        }
                    }

                    if (!foundSafe)
                    {
                        safePos = Vec2(100, 100);
                    }

                    player_->setPosition(safePos);
                }
                else
                {
                    player_->setPosition(centerPos);
                }
            }
        }
        else
        {
            auto visibleSize = Director::getInstance()->getVisibleSize();
            player_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
        }

        player_->enableKeyboardControl();

        if (mapLayer_)
        {
            player_->setMapLayer(mapLayer_);
        }

        this->addChild(player_, 10);
    }
    else
    {
        CCLOG("ERROR: Failed to create player!");
    }

}

// ========== 初始化摄像机 ==========

void GameScene::initCamera()

{

    CCLOG("Initializing camera...");

    // 获取默认摄像机

    auto camera = this->getDefaultCamera();

    if (camera && player_)

    {

        // 摄像机初始位置跟随玩家

        Vec2 playerPos = player_->getPosition();

        camera->setPosition(playerPos.x, playerPos.y);

        CCLOG("Camera initialized at player position");

    }

}

// ========== 初始化UI ==========

void GameScene::initUI()

{

    CCLOG("Initializing UI...");

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 创建UI层（独立于摄像机移动）

    uiLayer_ = Layer::create();

    uiLayer_->setGlobalZOrder(1000);  // 设置很高的z-order，确保在最上层

    this->addChild(uiLayer_, 1000);

    CCLOG("UI Layer created with global z-order 1000");

    // ===== 顶部信息栏背景 =====

    auto topBar = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, 60);

    topBar->setAnchorPoint(Vec2(0, 1));  // 锚点在左上角

    topBar->setPosition(Vec2(origin.x, origin.y + visibleSize.height));

    uiLayer_->addChild(topBar, 0);

    // ===== 时间显示 =====

    timeLabel_ = Label::createWithSystemFont("Day 1 (auto +1 every 5s)", "Arial", 24);

    timeLabel_->setAnchorPoint(Vec2(0, 0.5));

    timeLabel_->setPosition(Vec2(

        origin.x + 20,

        origin.y + visibleSize.height - 30

    ));

    timeLabel_->setColor(Color3B::WHITE);

    uiLayer_->addChild(timeLabel_, 1);

    // ===== 金币显示 =====

    moneyLabel_ = Label::createWithSystemFont("Gold: 500", "Arial", 24);

    moneyLabel_->setAnchorPoint(Vec2(1, 0.5));

    moneyLabel_->setPosition(Vec2(

        origin.x + visibleSize.width - 20,

        origin.y + visibleSize.height - 30

    ));

    moneyLabel_->setColor(Color3B(255, 215, 0));

    uiLayer_->addChild(moneyLabel_, 1);

    // ===== 位置显示（调试用）=====

    positionLabel_ = Label::createWithSystemFont("Position: (0, 0) | Tile: (0, 0)", "Arial", 18);

    positionLabel_->setPosition(Vec2(

        origin.x + visibleSize.width / 2,

        origin.y + visibleSize.height - 70

    ));

    positionLabel_->setColor(Color3B(200, 200, 200));

    uiLayer_->addChild(positionLabel_, 1);

    // ===== 操作提示 =====

    auto hint = Label::createWithSystemFont(
        "1-0: Switch item | J: Use | K: Water | B: Inventory | P: Market | M: Mine | ESC: Menu",
        "Arial", 18
    );

    hint->setPosition(Vec2(

        origin.x + visibleSize.width / 2,

        origin.y + 30

    ));

    hint->setColor(Color3B(200, 200, 200));

    uiLayer_->addChild(hint, 1);

    // ===== 农场操作提示 =====

    actionLabel_ = Label::createWithSystemFont("", "Arial", 20);

    actionLabel_->setPosition(Vec2(

        origin.x + visibleSize.width / 2,

        origin.y + 60

    ));

    actionLabel_->setColor(Color3B::WHITE);

    uiLayer_->addChild(actionLabel_, 1);

    // ===== 当前物品显示 =====
    itemLabel_ = Label::createWithSystemFont("Current item: Hoe (1-0 to switch)", "Arial", 18);
    itemLabel_->setAnchorPoint(Vec2(0, 0.5f));
    itemLabel_->setPosition(Vec2(
        origin.x + 20,
        origin.y + 60
    ));
    itemLabel_->setColor(Color3B(200, 255, 200));
    uiLayer_->addChild(itemLabel_, 1);

    initToolbar();

    // ===== 钓鱼 UI 初始化 (跟随玩家) =====
    if (player_)
    {
        chargeBarBg_ = Sprite::create();
        chargeBarBg_->setTextureRect(Rect(0, 0, 50, 8));
        chargeBarBg_->setColor(Color3B::GRAY);
        chargeBarBg_->setPosition(Vec2(16, 70)); 
        chargeBarBg_->setVisible(false);
        player_->addChild(chargeBarBg_);

        chargeBarFg_ = Sprite::create();
        chargeBarFg_->setTextureRect(Rect(0, 0, 0, 8));
        chargeBarFg_->setColor(Color3B::GREEN);
        chargeBarFg_->setAnchorPoint(Vec2(0, 0));
        chargeBarFg_->setPosition(Vec2(0, 0));
        chargeBarBg_->addChild(chargeBarFg_);

        exclamationMark_ = Sprite::create();
        exclamationMark_->setTextureRect(Rect(0, 0, 10, 30));
        exclamationMark_->setColor(Color3B::YELLOW);
        exclamationMark_->setPosition(Vec2(16, 90)); 
        exclamationMark_->setVisible(false);
        player_->addChild(exclamationMark_);
    }

    CCLOG("UI initialized - using simple fixed layer method");

}

// ========== Weather Init ==========

void GameScene::initWeather()

{

    if (!uiLayer_)

    {

        CCLOG("Weather skipped: uiLayer_ not initialized");

        return;

    }

    weatherManager_ = WeatherManager::create();

    if (weatherManager_)

    {

        uiLayer_->addChild(weatherManager_, -1);

        lastWeatherDay_ = 0;

        updateWeather();

        CCLOG("WeatherManager initialized");

    }

    else

    {

        CCLOG("ERROR: Failed to create WeatherManager!");

    }

}

// ========== 初始化控制 ==========

void GameScene::initControls()

{

    CCLOG("Initializing controls...");

    // ESC键监听（返回菜单）

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

    // 更新摄像机（跟随玩家）

    updateCamera();

    // 更新UI显示

    updateUI();

    updateWeather();

    // 处理砍树计时
    updateChopping(delta);

    // 更新钓鱼状态
    updateFishingState(delta);

}

void GameScene::updateCamera()

{

    if (!player_)

        return;

    auto camera = this->getDefaultCamera();

    if (!camera)

        return;

    // 获取玩家位置

    Vec2 playerPos = player_->getPosition();

    // 摄像机跟随玩家（平滑移动）

    Vec3 currentPos = camera->getPosition3D();

    Vec3 targetPos = Vec3(playerPos.x, playerPos.y, currentPos.z);

    // 线性插值实现平滑跟随

    float smoothFactor = 0.1f;  // 平滑系数（0-1，越大越快）

    Vec3 newPos = currentPos + (targetPos - currentPos) * smoothFactor;

    camera->setPosition3D(newPos);

    // 限制摄像机范围（不超出地图边界）

    if (mapLayer_)

    {

        auto visibleSize = Director::getInstance()->getVisibleSize();

        Size mapSize = mapLayer_->getMapSize();

        float minX = visibleSize.width / 2;

        float maxX = mapSize.width - visibleSize.width / 2;

        float minY = visibleSize.height / 2;

        float maxY = mapSize.height - visibleSize.height / 2;

        // 如果地图小于屏幕，居中显示

        if (mapSize.width < visibleSize.width)

        {

            minX = maxX = mapSize.width / 2;

        }

        if (mapSize.height < visibleSize.height)

        {

            minY = maxY = mapSize.height / 2;

        }

        Vec3 clampedPos = newPos;

        clampedPos.x = std::max(minX, std::min(maxX, newPos.x));

        clampedPos.y = std::max(minY, std::min(maxY, newPos.y));

        camera->setPosition3D(clampedPos);

    }

    // Update UI layer position so it stays with the camera (screen-space)

    if (uiLayer_)

    {

        Vec3 cameraPos = camera->getPosition3D();

        auto visibleSize = Director::getInstance()->getVisibleSize();

        // UI层的位置 = 摄像机位置 - 屏幕中心偏移

        Vec2 uiPos = Vec2(

            cameraPos.x - visibleSize.width / 2,

            cameraPos.y - visibleSize.height / 2

        );

        uiLayer_->setPosition(uiPos);

    }

}

void GameScene::updateUI()

{

    if (!player_)

        return;

    // 更新位置显示

    Vec2 playerPos = player_->getPosition();

    char posStr[64];

    sprintf(posStr, "Position: (%.0f, %.0f)", playerPos.x, playerPos.y);

    positionLabel_->setString(posStr);

    if (farmManager_)

    {

        std::string dayText = StringUtils::format("Day %d (auto +1 every 5s)", farmManager_->getDayCount());

        timeLabel_->setString(dayText);

    }

    if (moneyLabel_ && inventory_)
    {
        moneyLabel_->setString(StringUtils::format("Gold: %d", inventory_->getMoney()));
    }

    // 如果有地图层，也显示瓦片坐标

    if (mapLayer_)

    {

        Vec2 tileCoord = mapLayer_->positionToTileCoord(playerPos);

        sprintf(posStr, "Position: (%.0f, %.0f) | Tile: (%.0f, %.0f)",

            playerPos.x, playerPos.y, tileCoord.x, tileCoord.y);

        positionLabel_->setString(posStr);

    }

}

void GameScene::updateWeather()
{
    int dayCount = 1;
    if (farmManager_)
    {
        dayCount = farmManager_->getDayCount();
    }
    if (dayCount <= 0)
    {
        dayCount = 1;
    }
    if (dayCount == lastWeatherDay_)
    {
        return;
    }

    lastWeatherDay_ = dayCount;
    marketState_.updatePrices(dayCount);
    if (weatherManager_)
    {
        weatherManager_->updateWeather(marketState_.getWeather());
    }
}

void GameScene::handleFarmAction(bool waterOnly)
{
    if (!mapLayer_ || !farmManager_ || !player_)
        return;

    Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());
    tileCoord.x = std::round(tileCoord.x);
    tileCoord.y = std::round(tileCoord.y);

    FarmManager::ActionResult result{ false, "Unavailable", -1 };
    ItemType current = toolbarItems_.empty() ? ItemType::Hoe : toolbarItems_[selectedItemIndex_];

    if (waterOnly) {
        if (current == ItemType::WateringCan) {
            result = farmManager_->waterTile(tileCoord);
        }
        else {
            result = { false, "Need watering can to water", -1 };
        }
    }
    else {
        switch (current) {
        case ItemType::Hoe:
            result = farmManager_->tillTile(tileCoord);
            break;
        case ItemType::WateringCan:
            result = farmManager_->waterTile(tileCoord);
            break;
        case ItemType::Scythe:
            result = farmManager_->harvestTile(tileCoord);
            if (result.success && inventory_)
            {
                ItemType harvestItem = getItemTypeForCropId(result.cropId);
                if (harvestItem != ItemType::None)
                {
                    if (inventory_->addItem(harvestItem, 1))
                    {
                        result.message = StringUtils::format("Harvested %s (+1)", InventoryManager::getItemName(harvestItem).c_str());
                        if (inventoryUI_) inventoryUI_->refresh();
                    }
                    else
                    {
                        result.message = "Inventory full!";
                    }
                }
            }
            break;
         case ItemType::Axe: {
            // ========== 砍树逻辑（第3刀替换版）==========
            Vec2 target = tileCoord;
            if (!findNearbyCollisionTile(tileCoord, target)) {
                result = { false, "No tree to chop here", -1 };
                break;
            }

            std::vector<Vec2> component = collectCollisionComponent(target);
            if (component.empty() || component.size() > 10) {
                result = { false, "No tree to chop here", -1 };
                break;
            }

            // 查找是否已经在砍这棵树
            TreeChopData* existingChop = nullptr;
            for (auto& chop : activeChops_) {
                if (chop.tileCoord == component.front()) {
                    existingChop = &chop;
                    break;
                }
            }

            // 定义何时替换为 Sprite (第 3 刀)
            const int REPLACE_THRESHOLD = 3;

            if (existingChop) {
                // --- 已经在砍了 (第 2, 3, 4... 刀) ---
                existingChop->chopCount++;
                existingChop->chopTimer = 0.0f; // 重置超时计时

                // 【关键逻辑】如果是第 3 刀，且还没有 Sprite，立刻进行替换！
                if (existingChop->chopCount == REPLACE_THRESHOLD && existingChop->treeSprite == nullptr) {
                    CCLOG("第3刀！替换瓦片为Sprite...");
                    existingChop->treeSprite = createTreeSprite(existingChop->tiles); // 这会清除瓦片并生成Sprite
                    showActionMessage("Tree is loose!", Color3B::YELLOW);
                }

                // 如果 Sprite 已经存在（即 >= 3刀），播放摇晃动画
                if (existingChop->treeSprite) {
                    playTreeShakeAnimation(existingChop->treeSprite);
                }
                else {
                    // 如果还不到3刀（还是瓦片），只显示简单的撞击文字
                    Vec2 pos = mapLayer_->tileCoordToPosition(target);
                    // 简单的偏移，让字显示在上方
                    pos.x += mapLayer_->getTileSize().width / 2;
                    pos.y += mapLayer_->getTileSize().height;

                    // 这里由于没有Sprite拿不到位置，我们简单弹个全局字，或者你可以加个临时的Label
                    showActionMessage("Thump! (" + std::to_string(existingChop->chopCount) + ")", Color3B::WHITE);
                }

                // 检查是否砍倒 (假设需要 6 刀)
                if (existingChop->chopCount >= TreeChopData::CHOPS_NEEDED) {
                    if (existingChop->treeSprite) {
                        playTreeFallAnimation(existingChop);
                        result = { true, "Timber!", -1 };
                    }
                    else {
                        // 防御性编程：万一数值设置错了，没生成Sprite就倒了
                        result = { true, "Tree destroyed (No anim)", -1 };
                        // 这里应该补一个清理瓦片的逻辑，以防万一
                    }
                }
                else {
                    result = { true, "", -1 };
                }
            }
            else {
                // --- 第 1 刀 (新砍伐) ---
                TreeChopData newChop;
                newChop.tileCoord = component.front();
                newChop.tiles = component;
                newChop.chopCount = 1;
                newChop.chopTimer = 0.0f;

                // 【关键】第1刀不生成 Sprite，设为 nullptr
                newChop.treeSprite = nullptr;

                activeChops_.push_back(newChop);

                // 第1刀的反馈
                showActionMessage("Thump! (1)", Color3B::WHITE);
                result = { true, "", -1 };
            }
            break;
        }
        case ItemType::Pickaxe: {
            // Mine rocks: detect collision tile, verify rock GID, then clear it.
            Vec2 target = tileCoord;
            if (!findNearbyCollisionTile(tileCoord, target)) {
                result = { false, "No rock to mine here", -1 };
                break;
            }

            static const std::unordered_set<int> rockGids = {
                // Rock / ore tiles used in farm.tmx (bottom rows of tileset5)
                45019, 45020, 45021,
                45025, 45026, 45027, 45028, 45030,
                45069, 45070, 45071, 45072, 45073,
                45075, 45076, 45077, 45078, 45079, 45080, 45081,
                45125, 45126, 45157, 45170, 45171, 45172, 45173, 45175, 45176,
                45179, 45180, 45181, 45185,
                45220, 45223, 45224, 45225, 45226
            };

            int baseGid = mapLayer_->getBaseTileGID(target);
            if (rockGids.find(baseGid) == rockGids.end()) {
                result = { false, "No rock to mine here", -1 };
                break;
            }

            std::vector<Vec2> component = collectCollisionComponent(target);
            if (component.empty()) {
                result = { false, "No rock to mine here", -1 };
                break;
            }

            for (const auto& t : component) {
                mapLayer_->clearCollisionAt(t);
                mapLayer_->clearBaseTileAt(t);
            }

            result = { true, "Rock broken!", -1 };
            break;
        }
        case ItemType::SeedTurnip:
        case ItemType::SeedPotato:
        case ItemType::SeedCorn:
        case ItemType::SeedTomato:
        case ItemType::SeedPumpkin:
        case ItemType::SeedBlueberry: {
            if (!inventory_ || !inventory_->hasItem(current, 1))
            {
                result = { false, "No seeds left", -1 };
                break;
            }
            int cropId = getCropIdForItem(current);
            result = farmManager_->plantSeed(tileCoord, cropId);
            if (result.success && inventory_)
            {
                inventory_->removeItem(current, 1);
                result.message = StringUtils::format("Planted %s (-1)", InventoryManager::getItemName(current).c_str());
                if (inventoryUI_) inventoryUI_->refresh();
            }
            break;
        }
        default:
            result = { false, "Unknown item action", -1 };
            break;
        }
    }

    Color3B color = result.success ? Color3B(120, 230, 140) : Color3B(255, 180, 120);
    showActionMessage(result.message, color);
}

// ========== 砍树相关函数 ==========

// ========== 辅助函数定义 (必须在 handleFarmAction 外部) ==========

std::vector<Vec2> GameScene::collectCollisionComponent(const Vec2& start) const //收集所有连在一起的树木瓦片
{
    std::vector<Vec2> out;
    if (!mapLayer_ || !mapLayer_->hasCollisionAt(start))
        return out;

    // 【关键】定义树木的 GID 白名单（3x3树的所有GID）
    // 顶部: 43557-43559, 中间: 43607-43609, 底部: 43657-43659
    static const std::vector<int> treeGids = {
        43557, 43558, 43559,  // 顶部
        43607, 43608, 43609,  // 中间
        43657, 43658, 43659   // 底部（树根层）
    };

    // 检查起始点是否是树（防止砍到石头）
    int startGid = mapLayer_->getBaseTileGID(start);
    bool isStartTree = false;
    for (int id : treeGids) {
        if (id == startGid) {
            isStartTree = true;
            break;
        }
    }

    if (!isStartTree)
        return out; // 如果起始点不是树，直接返回空

    Size mapSize = mapLayer_->getMapSizeInTiles();
    auto key = [](int x, int y) -> long long {
        return (static_cast<long long>(y) << 32) |
            (static_cast<unsigned long long>(x) & 0xffffffffULL);
        };

    std::queue<Vec2> q;
    std::unordered_set<long long> visited;

    q.push(start);
    visited.insert(key(static_cast<int>(start.x), static_cast<int>(start.y)));

    const Vec2 dirs[4] = { Vec2(1, 0), Vec2(-1, 0), Vec2(0, 1), Vec2(0, -1) };

    while (!q.empty()) {
        Vec2 t = q.front();
        q.pop();
        out.push_back(t);

        for (const auto& d : dirs) {
            int nx = static_cast<int>(t.x + d.x);
            int ny = static_cast<int>(t.y + d.y);

            // 边界检查
            if (nx < 0 || ny < 0 || nx >= mapSize.width || ny >= mapSize.height)
                continue;

            long long k = key(nx, ny);
            if (visited.count(k))
                continue;

            Vec2 nt(static_cast<float>(nx), static_cast<float>(ny));

            // 【关键修改】同时检查：1.是否有碰撞 2.是否是树木ID
            if (mapLayer_->hasCollisionAt(nt)) {
                int nextGid = mapLayer_->getBaseTileGID(nt);
                bool isNextTree = false;
                for (int id : treeGids) {
                    if (id == nextGid) {
                        isNextTree = true;
                        break;
                    }
                }

                if (isNextTree) {
                    visited.insert(k);
                    q.push(nt);
                }
            }
        }
    }

    return out;
}

bool GameScene::findNearbyCollisionTile(const Vec2& centerTile, Vec2& outTile) const {
    if (!mapLayer_)
        return false;

    // 只检测3个方向：左、右、脚下
    const Vec2 offsets[3] = {
        Vec2(0, 0),   // 玩家脚下
        Vec2(1, 0),   // 右边
        Vec2(-1, 0)   // 左边
    };

    const int TREE_ROOT_GID = 43658; // 树根的实际GID (tileset id=901)

    for (const auto& off : offsets) {
        Vec2 candidate = centerTile + off;

        // 检查是否有碰撞 AND GID是否是树根
        if (mapLayer_->hasCollisionAt(candidate)) {
            int gid = mapLayer_->getBaseTileGID(candidate);
            if (gid == TREE_ROOT_GID) {
                outTile = candidate;
                CCLOG("发现树根于位置: (%.0f, %.0f), GID=%d", candidate.x, candidate.y, gid);
                return true;
            }
        }
    }

    return false;
}

Sprite* GameScene::createTreeSprite(const std::vector<Vec2>& tiles) {
    if (tiles.empty() || !mapLayer_)
        return nullptr;

    Size tileSize = mapLayer_->getTileSize();

    // ==========================================
    // 1. 找到真正的树根（GID = 43658）
    // ==========================================
    Vec2 rootTile(-1, -1);
    const int TREE_ROOT_GID = 43658; // 树根的实际GID

    for (const auto& t : tiles) {
        int gid = mapLayer_->getBaseTileGID(t);
        if (gid == TREE_ROOT_GID) {
            rootTile = t;
            break;
        }
    }

    // 如果没找到树根GID，使用Y最大的作为fallback
    if (rootTile.x < 0) {
        rootTile = tiles[0];
        for (const auto& t : tiles) {
            if (t.y > rootTile.y) rootTile = t;
        }
        CCLOG("Warning: Tree root GID 43658 not found, using fallback position");
    }

    // 算出树根格子在屏幕上的像素位置
    Vec2 rootTilePos = mapLayer_->tileCoordToPosition(rootTile);

    // 计算出生点：树根格子的【底边中点】
    Vec2 spawnPosition = Vec2(rootTilePos.x + tileSize.width / 2.0f, rootTilePos.y);

    // ==========================================
    // ==========================================
    // 2. 强制清理整棵树的3x3范围
    // ==========================================
    int rootTx = static_cast<int>(rootTile.x);
    int rootTy = static_cast<int>(rootTile.y);

    // 采样草地颜色
    int grassGID = 1; // 默认值

    // 【修改点】不再取树根右边，而是取地图上固定的安全草地坐标 (30, 14)
    Vec2 samplePos = Vec2(30, 14);

    int detectedGID = mapLayer_->getBaseTileGID(samplePos);
    if (detectedGID > 0) grassGID = detectedGID;

    // 清理3x3范围：以树根为基准，左右各1格，向上2格
    for (int x = rootTx - 1; x <= rootTx + 1; ++x) {
        for (int y = rootTy - 2; y <= rootTy; ++y) {
            Vec2 target(x, y);

            if (x >= 0 && y >= 0 &&
                x < mapLayer_->getMapSize().width &&
                y < mapLayer_->getMapSize().height)
            {
                mapLayer_->setBaseTileGID(target, grassGID);
            }
        }
    }

    // ==========================================
    // 3. 创建并强制缩放替身 (96x96)
    // ==========================================
    auto treeSprite = Sprite::create("images/items/tree_full.png");
    if (!treeSprite) {
        CCLOG("Error: tree_full.png not found!");
        return nullptr;
    }

    float targetWidth = 96.0f;
    float targetHeight = 96.0f;
    Size textureSize = treeSprite->getContentSize();

    float scaleX = targetWidth / textureSize.width;
    float scaleY = targetHeight / textureSize.height;

    treeSprite->setScaleX(scaleX);
    treeSprite->setScaleY(scaleY);

    // ==========================================
    // 4. 关键定位（锚点在底部中心）
    // ==========================================
    treeSprite->setAnchorPoint(Vec2(0.5f, 0.0f));
    treeSprite->setPosition(spawnPosition);

    // 保存树根位置到sprite的userData，方便后续放树桩
    treeSprite->setUserData((void*)new Vec2(rootTile));

    this->addChild(treeSprite, 100);

    CCLOG("生成树木: 树根(%d, %d) GID=43658 | 清理3x3 | 尺寸96x96", rootTx, rootTy);

    return treeSprite;
}

void GameScene::playTreeShakeAnimation(Sprite* treeSprite) {
    if (!treeSprite)
        return;

    // 停止之前的动画
    treeSprite->stopAllActions();

    // 增强版震动动画：更长时间、更大幅度
    auto shake1 = RotateTo::create(0.08f, -12);   // 增加角度和时间
    auto shake2 = RotateTo::create(0.08f, 12);
    auto shake3 = RotateTo::create(0.08f, -10);
    auto shake4 = RotateTo::create(0.08f, 10);
    auto shake5 = RotateTo::create(0.08f, -6);
    auto shake6 = RotateTo::create(0.08f, 6);
    auto shake7 = RotateTo::create(0.08f, -3);
    auto shake8 = RotateTo::create(0.08f, 3);
    auto shake9 = RotateTo::create(0.08f, 0);

    // 增加树叶飘落效果（可选）
    auto scaleUp = ScaleTo::create(0.1f, 1.05f, 1.05f);
    auto scaleDown = ScaleTo::create(0.1f, 1.0f, 1.0f);
    auto scaleSeq = Sequence::create(scaleUp, scaleDown, nullptr);

    auto shakeSeq = Sequence::create(
        shake1, shake2, shake3, shake4, shake5,
        shake6, shake7, shake8, shake9, nullptr
    );

    // 同时执行摇晃和缩放
    auto spawn = Spawn::create(shakeSeq, scaleSeq, nullptr);
    treeSprite->runAction(spawn);

    // 添加震动提示文字
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto camera = this->getDefaultCamera();
    Vec3 camPos = camera->getPosition3D();

    auto hitLabel = Label::createWithSystemFont("CHOP!", "Arial", 30);
    hitLabel->setPosition(Vec2(
        treeSprite->getPosition().x,
        treeSprite->getPosition().y + 120
    ));
    hitLabel->setColor(Color3B(255, 100, 100));
    this->addChild(hitLabel, 200);

    auto labelSeq = Sequence::create(
        ScaleTo::create(0.1f, 1.3f),
        ScaleTo::create(0.1f, 1.0f),
        DelayTime::create(0.3f),
        FadeOut::create(0.3f),
        RemoveSelf::create(),
        nullptr
    );
    hitLabel->runAction(labelSeq);
}

void GameScene::playTreeFallAnimation(TreeChopData* chopData) {
    if (!chopData || !chopData->treeSprite)
        return;

    auto treeSprite = chopData->treeSprite;

    // 保存关键数据
    Vec2 savedTileCoord = chopData->tileCoord;
    std::vector<Vec2> savedTiles = chopData->tiles;

    // 从sprite的userData中获取真正的树根位置
    Vec2* pRootTile = static_cast<Vec2*>(treeSprite->getUserData());
    Vec2 actualRootTile = pRootTile ? *pRootTile : savedTileCoord;

    // ==========================================
    // 增强版倒下动画
    // ==========================================

    // 第一阶段：树开始倾斜（0.3秒）
    auto tiltStart = RotateTo::create(0.3f, -15);

    // 第二阶段：加速倒下（0.8秒）
    auto fallDown = RotateTo::create(0.8f, 90);

    // 第三阶段：砸地反弹效果（0.2秒）
    auto bounce = Sequence::create(
        RotateTo::create(0.1f, 95),  // 稍微过头
        RotateTo::create(0.1f, 90),  // 反弹回来
        nullptr
    );

    // 位置动画：树倒下时会向右移动
    Vec2 currentPos = treeSprite->getPosition();
    auto moveRight = MoveTo::create(0.8f, Vec2(currentPos.x + 48, currentPos.y));

    // 缩放动画：倒下时稍微压扁（增加重量感）
    auto scaleSeq = Sequence::create(
        DelayTime::create(0.3f),
        ScaleTo::create(0.8f, 1.0f, 0.95f),  // 稍微压扁
        nullptr
    );

    // 淡出延迟：等树完全倒下后再淡出
    auto fadeSeq = Sequence::create(
        DelayTime::create(1.1f),  // 等倒下动画完成
        FadeOut::create(0.5f),    // 慢慢淡出
        nullptr
    );

    // 组合所有动画
    auto rotateSeq = Sequence::create(tiltStart, fallDown, bounce, nullptr);
    auto spawnAnim = Spawn::create(rotateSeq, moveRight, scaleSeq, fadeSeq, nullptr);

    // ==========================================
    // 添加特效
    // ==========================================

    // "TIMBER!" 文字特效
    auto timberLabel = Label::createWithSystemFont("TIMBER!", "Arial", 40);
    timberLabel->setPosition(Vec2(
        treeSprite->getPosition().x,
        treeSprite->getPosition().y + 150
    ));
    timberLabel->setColor(Color3B(255, 200, 50));
    this->addChild(timberLabel, 200);

    auto labelAnim = Sequence::create(
        Spawn::create(
            ScaleTo::create(0.3f, 1.5f),
            JumpBy::create(0.3f, Vec2(0, 0), 30, 1),
            nullptr
        ),
        DelayTime::create(0.5f),
        FadeOut::create(0.5f),
        RemoveSelf::create(),
        nullptr
    );
    timberLabel->runAction(labelAnim);


    // ==========================================
    // 清理工作（延迟执行）
    // ==========================================
    auto cleanup = CallFunc::create([this, actualRootTile, savedTiles, treeSprite]() {
        // --- A. 清理userData ---
        Vec2* pData = static_cast<Vec2*>(treeSprite->getUserData());
        if (pData) {
            delete pData;
            treeSprite->setUserData(nullptr);
        }

        // --- B. 计算树桩位置（只在真正的树根位置）---
        Vec2 rootTilePos = mapLayer_->tileCoordToPosition(actualRootTile);
        Size tileSize = mapLayer_->getTileSize();
        Vec2 stumpPos = Vec2(rootTilePos.x + tileSize.width / 2.0f, rootTilePos.y);

        // --- C. 移除倒下的树 ---
        treeSprite->removeFromParent();

        // --- D. 生成【唯一】树桩（只在树根GID=43658的位置）---
        auto stump = Sprite::create("images/items/tree_stump.png");
        if (stump) {
            stump->setPosition(stumpPos);
            stump->setAnchorPoint(Vec2(0.5f, 0.0f));

            // 树桩缩放到32x32（1个tile大小）
            Size stumpSize = stump->getContentSize();
            float stumpScale = 32.0f / std::max(stumpSize.width, stumpSize.height);
            stump->setScale(stumpScale);

            mapLayer_->addChild(stump, 5);

            // 树桩出现动画
            stump->setOpacity(0);
            stump->runAction(FadeIn::create(0.3f));

            CCLOG("树桩已放置在树根位置: (%d, %d)",
                static_cast<int>(actualRootTile.x),
                static_cast<int>(actualRootTile.y));
        }

        // --- E. 掉落木材（在树桩位置）---
        int woodCount = 3 + (rand() % 3);
        spawnItem(ItemType::Wood, stumpPos, woodCount);

        // --- F. 记录被砍倒的树木位置（用于存档） ---
        // 记录整棵树的所有瓦片
        for (const auto& tile : savedTiles)
        {
            // 检查是否已记录
            auto it = std::find(choppedTrees_.begin(), choppedTrees_.end(), tile);
            if (it == choppedTrees_.end())
            {
                choppedTrees_.push_back(tile);
                CCLOG("Recorded chopped tree tile: (%.0f, %.0f)", tile.x, tile.y);
            }
        }

        // --- G. 清理砍树数据 ---
        activeChops_.erase(
            std::remove_if(activeChops_.begin(), activeChops_.end(),
                [actualRootTile](const TreeChopData& c) {
                    return c.tileCoord == actualRootTile;
                }),
            activeChops_.end());

        showActionMessage("Tree chopped! Got wood!", Color3B(200, 255, 200));
        });

    // 执行完整动画序列
    auto fullSequence = Sequence::create(
        spawnAnim,              // 倒下动画（1.6秒）
        DelayTime::create(0.3f), // 等0.3秒让淡出完成
        cleanup,                // 清理和生成树桩
        nullptr
    );

    treeSprite->runAction(fullSequence);
}

void GameScene::spawnItem(ItemType type, const Vec2& position, int count) {
    // 创建掉落物精灵
    std::string itemImage;
    switch (type) {
    case ItemType::Wood:
        itemImage = "items/wood.png"; // ← 需要这个PNG
        break;
    default:
        itemImage = "items/unknown.png";
        break;
    }

    auto item = Sprite::create(itemImage);
    if (!item) {
        // 如果没有图片，创建占位符
        item = Sprite::create();
        auto drawNode = DrawNode::create();
        drawNode->drawSolidCircle(Vec2::ZERO, 10, 0, 16,
            Color4F(0.6f, 0.4f, 0.2f, 1.0f));
        item->addChild(drawNode);
    }

    item->setPosition(position);
    this->addChild(item, 50);

    // 掉落动画：向上抛起后落下
    auto jumpUp = JumpBy::create(0.5f, Vec2(0, 0), 30, 1);
    auto fadeOut = FadeOut::create(0.3f);
    auto remove = RemoveSelf::create();

    auto sequence = Sequence::create(jumpUp, fadeOut, remove, nullptr);
    item->runAction(sequence);

    // 显示数量标签
    if (count > 1) {
        auto countLabel = Label::createWithSystemFont(
            StringUtils::format("+%d", count), "Arial", 20);
        countLabel->setPosition(Vec2(position.x, position.y + 40));
        countLabel->setColor(Color3B(255, 255, 100));
        this->addChild(countLabel, 51);

        auto labelFade =
            Sequence::create(DelayTime::create(0.5f), FadeOut::create(0.5f),
                RemoveSelf::create(), nullptr);
        countLabel->runAction(labelFade);
    }

    // TODO: 实际游戏中应该将物品添加到背包系统
    CCLOG("Collected %d x %s", count, getItemName(type).c_str());
}

void GameScene::updateChopping(float delta) {
    // 这个函数现在主要用于清理超时的砍树任务（可选）
    // 新版本中通过点击次数触发，不需要倒计时

    // 可选：添加超时机制（如果10秒内没继续砍，自动取消）
    for (int i = static_cast<int>(activeChops_.size()) - 1; i >= 0; --i) {
        activeChops_[i].chopTimer += delta;

        // 如果超过10秒没继续砍，取消砍树
        if (activeChops_[i].chopTimer > 10.0f) {
            // 恢复瓦片
            for (const auto& tile : activeChops_[i].tiles) {
                // 这里需要恢复原始 GID，你可能需要在 TreeChopData 中保存
            }

            // 移除精灵
            if (activeChops_[i].treeSprite) {
                activeChops_[i].treeSprite->removeFromParent();
            }

            activeChops_.erase(activeChops_.begin() + i);
            showActionMessage("Chopping cancelled (timeout)", Color3B(200, 200, 200));
        }
    }
}

// ========== 树木调试显示 ==========
void GameScene::initTrees() {
    trees_.clear();
    activeChops_.clear();

    if (!mapLayer_)
        return;

    // 这里可以放置一些调试用的树木标记
    // 实际游戏中树木应该从地图中读取
    std::vector<Vec2> sampleTiles = { Vec2(8, 8), Vec2(10, 12), Vec2(12, 9) };

    Size tileSize = mapLayer_->getTileSize();
    float halfW = tileSize.width / 2.0f;
    float halfH = tileSize.height / 2.0f;

    for (const auto& t : sampleTiles) {
        Vec2 pos = mapLayer_->tileCoordToPosition(t);
        auto node = DrawNode::create();
        Vec2 bl(pos.x - halfW + 2, pos.y - halfH + 2);
        Vec2 tr(pos.x + halfW - 2, pos.y + halfH - 2);
        node->drawSolidRect(bl, tr, Color4F(0.2f, 0.6f, 0.2f, 0.9f));
        node->drawSolidCircle(pos, 10.0f, 0, 12, 1.0f, 1.0f,
            Color4F(0.1f, 0.5f, 0.1f, 0.9f));
        mapLayer_->addChild(node, 15);

        trees_.push_back(Tree{ t, node });
    }
}

int GameScene::findTreeIndex(const Vec2& tile) const {
    for (int i = 0; i < static_cast<int>(trees_.size()); ++i) {
        if (trees_[i].tileCoord == tile)
            return i;
    }
    return -1;
}

void GameScene::showActionMessage(const std::string& text, const Color3B& color)

{

    if (!actionLabel_)

        return;

    actionLabel_->setString(text);

    actionLabel_->setColor(color);

    actionLabel_->stopAllActions();

    actionLabel_->setOpacity(255);

    auto seq = Sequence::create(

        DelayTime::create(0.2f),

        FadeOut::create(1.0f),

        nullptr);

    actionLabel_->runAction(seq);

}

void GameScene::initToolbar()

{

    toolbarItems_ = {
        ItemType::Hoe,
        ItemType::WateringCan,
        ItemType::Scythe,
        ItemType::Axe,
        ItemType::Pickaxe,
        ItemType::FishingRod, // ID 5 (index 5)
        ItemType::SeedTurnip,
        ItemType::SeedPotato,
        ItemType::SeedCorn,
        ItemType::SeedTomato,
        ItemType::SeedPumpkin,
        ItemType::SeedBlueberry
    };

    selectedItemIndex_ = 0;
    selectItemByIndex(0);

}

void GameScene::selectItemByIndex(int idx)

{

    if (toolbarItems_.empty())
        return;

    if (idx < 0 || idx >= static_cast<int>(toolbarItems_.size()))
        return;

    selectedItemIndex_ = idx;
    std::string name = InventoryManager::getItemName(toolbarItems_[selectedItemIndex_]);

    if (itemLabel_)
    {
        itemLabel_->setString(StringUtils::format("Current item: %s (1-0/-/= to switch)", name.c_str()));
    }

    showActionMessage(StringUtils::format("Switched to %s", name.c_str()), Color3B(180, 220, 255));
}


int GameScene::getCropIdForItem(ItemType type) const
{
    switch (type)
    {
    case ItemType::SeedTurnip: return 0;
    case ItemType::SeedPotato: return 1;
    case ItemType::SeedCorn: return 2;
    case ItemType::SeedTomato: return 3;
    case ItemType::SeedPumpkin: return 4;
    case ItemType::SeedBlueberry: return 5;
    default: return -1;
    }
}

ItemType GameScene::getItemTypeForCropId(int cropId) const
{
    switch (cropId)
    {
    case 0: return ItemType::Turnip;
    case 1: return ItemType::Potato;
    case 2: return ItemType::Corn;
    case 3: return ItemType::Tomato;
    case 4: return ItemType::Pumpkin;
    case 5: return ItemType::Blueberry;
    default: return ItemType::None;
    }
}

void GameScene::onMouseDown(Event* event)
{
    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
    {
        // Check current item
        if (toolbarItems_.empty()) return;
        
        ItemType current = ItemType::Hoe;
        if (selectedItemIndex_ >= 0 && selectedItemIndex_ < (int)toolbarItems_.size()) {
            current = toolbarItems_[selectedItemIndex_];
        }

        // Fishing Rod Logic
        if (current == ItemType::FishingRod)
        {
            if (fishingState_ == FishingState::NONE)
            {
                 if (isFishing_) return;
                 fishingState_ = FishingState::CHARGING;
                 chargePower_ = 0.0f;
                 CCLOG("Start Charging...");
            }
            else if (fishingState_ == FishingState::BITING)
            {
                 CCLOG("HOOKED!");
                 if (exclamationMark_) exclamationMark_->setVisible(false);
                 startFishing(); 
            }
            else if (fishingState_ == FishingState::WAITING)
            {
                 CCLOG("Pulled too early!");
                 fishingState_ = FishingState::NONE;
                 showActionMessage("Too early!", Color3B::RED);
            }
            return; 
        }

        // Mouse click for farming? 
        // The new architecture uses Keyboard J/K for farm actions.
        // But we can keep mouse click for debugging or if user wants it.
        // For now, let's just log the click debug info the user liked.
        
        Vec2 clickPos = e->getLocationInView();
        clickPos.y = Director::getInstance()->getWinSize().height - clickPos.y; 
        Vec3 cameraPos = this->getDefaultCamera()->getPosition3D();
        Vec2 worldPos = clickPos + Vec2(cameraPos.x, cameraPos.y) - Director::getInstance()->getVisibleSize() / 2;
        
        CCLOG("Click Debug: Screen(%.1f, %.1f) -> World(%.1f, %.1f)", clickPos.x, clickPos.y, worldPos.x, worldPos.y);
        if (mapLayer_) {
            Vec2 t = mapLayer_->positionToTileCoord(worldPos);
            CCLOG("Target Tile: (%.0f, %.0f)", t.x, t.y);
        }
    }
}

void GameScene::onMouseUp(Event* event)
{
    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
    {
        if (fishingState_ == FishingState::CHARGING)
        {
            fishingState_ = FishingState::WAITING;
            if (chargeBarBg_) chargeBarBg_->setVisible(false);
            
            // Wait 1.0 - 4.0s
            waitTimer_ = CCRANDOM_0_1() * 3.0f + 1.0f;
            CCLOG("Casting rod! Power: %.2f. Waiting...", chargePower_);
        }
    }
}

void GameScene::updateFishingState(float delta)
{
    if (fishingState_ == FishingState::CHARGING)
    {
        chargePower_ += delta * 1.5f; 
        if (chargePower_ > 1.0f) chargePower_ = 1.0f;
        
        if (chargeBarBg_) chargeBarBg_->setVisible(true);
        if (chargeBarFg_) {
            chargeBarFg_->setTextureRect(Rect(0, 0, 50 * chargePower_, 8));
            chargeBarFg_->setColor(Color3B(255 * chargePower_, 255 * (1-chargePower_) + 100, 0));
        }
    }
    else if (fishingState_ == FishingState::WAITING)
    {
        waitTimer_ -= delta;
        if (waitTimer_ <= 0)
        {
            fishingState_ = FishingState::BITING;
            biteTimer_ = 1.0f; 
            if (exclamationMark_) exclamationMark_->setVisible(true);
            CCLOG("Fish bit! Click now!");
        }
    }
    else if (fishingState_ == FishingState::BITING)
    {
        biteTimer_ -= delta;
        if (biteTimer_ <= 0)
        {
            fishingState_ = FishingState::NONE;
            if (exclamationMark_) exclamationMark_->setVisible(false);
            CCLOG("Missed...");
            showActionMessage("Missed...", Color3B::GRAY);
        }
    }
}

void GameScene::startFishing()
{
    isFishing_ = true;
    fishingState_ = FishingState::REELING; 

    if (player_) player_->setMoveSpeed(0); // Using new setMoveSpeed API

    auto fishingLayer = FishingLayer::create();
    fishingLayer->setFinishCallback([this](bool success) {
        this->isFishing_ = false;
        this->fishingState_ = FishingState::NONE; 
        
        if (this->chargeBarBg_) this->chargeBarBg_->setVisible(false);
        if (this->exclamationMark_) this->exclamationMark_->setVisible(false);
        if (this->player_) this->player_->setMoveSpeed(150.0f); // Restore speed (was hardcoded, assumed 150)

        if (success)
        {
            CCLOG("Fishing SUCCESS!");
            showActionMessage("Caught a Fish!", Color3B(255, 215, 0));
            // No inventory addItem API anymore? 
            // We can't add item to Toolbar easily if it's full/fixed.
            // For now just show message.
        }
        else
        {
            CCLOG("Fishing FAILED.");
            showActionMessage("Fish got away...", Color3B::RED);
        }
    });

    this->addChild(fishingLayer, 100);
}
// ========== 返回菜单 ==========

void GameScene::backToMenu()

{

    CCLOG("Returning to menu...");
    saveGame();

    auto scene = MenuScene::createScene();

    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, scene));

}

// ========== 背包系统相关 ==========

void GameScene::toggleInventory()
{
    if (!inventory_)
    {
        CCLOG("ERROR: Inventory not initialized!");
        return;
    }

    // 如果背包已经打开，关闭它
    if (inventoryUI_)
    {
        inventoryUI_->close();
        return;
    }

    // 创建背包UI
    inventoryUI_ = InventoryUI::create(inventory_);
    if (!inventoryUI_)
    {
        CCLOG("ERROR: Failed to create InventoryUI!");
        return;
    }

    inventoryUI_->setCloseCallback([this]() {
        onInventoryClosed();
    });

    if (uiLayer_)
    {
        uiLayer_->addChild(inventoryUI_, 2000);
        inventoryUI_->setPosition(Vec2::ZERO);
    }
    else
    {
        this->addChild(inventoryUI_, 2000);
    }

    inventoryUI_->show();
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

Scene* GameScene::createScene(bool loadFromSave)
{
    auto scene = GameScene::create();
    if (scene && loadFromSave)
    {
        // 通过调用带参数的 init 来加载存档
        // 但由于 create() 已经调用了 init()，我们需要手动加载
        GameScene* gameScene = dynamic_cast<GameScene*>(scene);
        if (gameScene)
        {
            gameScene->loadGame();
        }
    }
    return scene;
}

bool GameScene::init(bool loadFromSave)
{
    if (!init())
        return false;

    if (loadFromSave)
    {
        loadGame();
    }

    return true;
}

void GameScene::saveGame()
{
    CCLOG("========================================");
    CCLOG("Saving game...");
    CCLOG("========================================");

    SaveManager::SaveData data = collectSaveData();

    if (SaveManager::getInstance()->saveGame(data))
    {
        CCLOG("✓ Game saved successfully!");
        // 显示保存成功提示
        showActionMessage("Game Saved!", Color3B::GREEN);
    }
    else
    {
        CCLOG("✗ Failed to save game!");
        showActionMessage("Save Failed!", Color3B::RED);
    }
}

void GameScene::loadGame()
{
    CCLOG("========================================");
    CCLOG("Loading game...");
    CCLOG("========================================");

    SaveManager::SaveData data;

    if (SaveManager::getInstance()->loadGame(data))
    {
        CCLOG("✓ Game loaded successfully from file!");
        applySaveData(data);

        // 延迟显示消息，确保 UI 已初始化
        if (actionLabel_)
        {
            showActionMessage("Game Loaded!", Color3B::GREEN);
        }
    }
    else
    {
        CCLOG("✗ Failed to load game!");
        if (actionLabel_)
        {
            showActionMessage("Load Failed!", Color3B::RED);
        }
    }
}

SaveManager::SaveData GameScene::collectSaveData()
{
    SaveManager::SaveData data;

    // 保存玩家位置
    if (player_)
    {
        data.playerPosition = player_->getPosition();
        CCLOG("Saving player position: (%.2f, %.2f)",
            data.playerPosition.x, data.playerPosition.y);
    }

    // 保存背包数据
    if (inventory_)
    {
        data.inventory.money = inventory_->getMoney();
        CCLOG("Saving money: %d", data.inventory.money);

        const auto& slots = inventory_->getAllSlots();
        for (size_t i = 0; i < slots.size(); i++)
        {
            const auto& slot = slots[i];
            if (!slot.isEmpty())
            {
                SaveManager::SaveData::InventoryData::ItemSlotData slotData;
                slotData.type = static_cast<int>(slot.type);
                slotData.count = slot.count;
                data.inventory.slots.push_back(slotData);
                CCLOG("  Slot %zu: Type=%d, Count=%d", i, slotData.type, slotData.count);
            }
            else
            {
                // 空槽位也要保存，保持索引一致
                SaveManager::SaveData::InventoryData::ItemSlotData slotData;
                slotData.type = static_cast<int>(ItemType::None);
                slotData.count = 0;
                data.inventory.slots.push_back(slotData);
            }
        }
    }

    // 保存游戏天数
    if (farmManager_)
    {
        data.dayCount = farmManager_->getDayCount();
        CCLOG("Saving day count: %d", data.dayCount);
    }

    // 保存农作物数据
    if (farmManager_)
    {
        const auto& farmTiles = farmManager_->getAllTiles();
        Size mapSize = farmManager_->getMapSize();

        for (int y = 0; y < mapSize.height; y++)
        {
            for (int x = 0; x < mapSize.width; x++)
            {
                int index = y * mapSize.width + x;
                if (index >= farmTiles.size())
                    continue;

                const auto& tile = farmTiles[index];

                // 只保存有状态的瓦片
                if (tile.tilled || tile.hasCrop)
                {
                    SaveManager::SaveData::FarmTileData tileData;
                    tileData.x = x;
                    tileData.y = y;
                    tileData.tilled = tile.tilled;
                    tileData.watered = tile.watered;
                    tileData.hasCrop = tile.hasCrop;
                    tileData.cropId = tile.cropId;
                    tileData.stage = tile.stage;
                    tileData.progressDays = tile.progressDays;
                    data.farmTiles.push_back(tileData);
                }
            }
        }
        CCLOG("Saving %zu farm tiles", data.farmTiles.size());
    }

    // 树木存档功能已禁用（避免崩溃问题）
    // 注意：砍倒的树木在重新加载游戏后会恢复

    return data;
}

void GameScene::applySaveData(const SaveManager::SaveData& data)
{
    CCLOG("========================================");
    CCLOG("Applying save data...");
    CCLOG("========================================");

    // 恢复玩家位置
    if (player_)
    {
        CCLOG("Restoring player position...");
        player_->setPosition(data.playerPosition);
        CCLOG("✓ Player position restored: (%.2f, %.2f)",
            data.playerPosition.x, data.playerPosition.y);
    }
    else
    {
        CCLOG("✗ Warning: player_ is null!");
    }

    // 恢复背包数据
    if (inventory_)
    {
        CCLOG("Restoring inventory...");
        // 清空现有背包
        inventory_->clear();

        // 恢复金币 - 使用 addMoney 一次性添加
        if (data.inventory.money > 0)
        {
            inventory_->addMoney(data.inventory.money);
        }
        CCLOG("✓ Money restored: %d", data.inventory.money);

        // 恢复物品槽位
        CCLOG("Restoring %zu inventory slots...", data.inventory.slots.size());
        for (size_t i = 0; i < data.inventory.slots.size() && i < InventoryManager::MAX_SLOTS; i++)
        {
            const auto& slotData = data.inventory.slots[i];
            if (slotData.type != static_cast<int>(ItemType::None) && slotData.count > 0)
            {
                ItemType type = static_cast<ItemType>(slotData.type);
                inventory_->setSlot(i, type, slotData.count);
                CCLOG("  Slot %zu restored: Type=%d, Count=%d", i, slotData.type, slotData.count);
            }
        }
        CCLOG("✓ Inventory restored");
    }
    else
    {
        CCLOG("✗ Warning: inventory_ is null!");
    }

    // 恢复游戏天数
    if (farmManager_)
    {
        CCLOG("Restoring day count...");
        farmManager_->setDayCount(data.dayCount);
        CCLOG("✓ Day count restored: %d", data.dayCount);
    }
    else
    {
        CCLOG("✗ Warning: farmManager_ is null!");
    }

    // 恢复农作物数据
    if (farmManager_)
    {
        CCLOG("Restoring farm tiles...");
        // 获取当前地图尺寸
        Size mapSize = farmManager_->getMapSize();
        std::vector<FarmManager::FarmTile> tiles(mapSize.width * mapSize.height);

        // 应用保存的瓦片数据
        for (const auto& tileData : data.farmTiles)
        {
            int index = tileData.y * mapSize.width + tileData.x;
            if (index >= 0 && index < tiles.size())
            {
                tiles[index].tilled = tileData.tilled;
                tiles[index].watered = tileData.watered;
                tiles[index].hasCrop = tileData.hasCrop;
                tiles[index].cropId = tileData.cropId;
                tiles[index].stage = tileData.stage;
                tiles[index].progressDays = tileData.progressDays;
            }
        }

        farmManager_->setAllTiles(tiles);
        CCLOG("✓ Farm tiles restored: %zu tiles", data.farmTiles.size());
    }

    // 树木恢复功能已禁用（避免崩溃问题）
    // 注意：砍倒的树木在重新加载游戏后会恢复
    CCLOG("Tree restoration skipped (feature disabled for stability)");
    choppedTrees_.clear();

    CCLOG("========================================");
    CCLOG("✓ Save data applied successfully!");
    CCLOG("========================================");
}
