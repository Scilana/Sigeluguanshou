#include "GameScene.h"
#include "MenuScene.h"
#include "FarmManager.h"

USING_NS_CC;

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
    uiLayer_ = nullptr;

    initMap();
    initFarm();
    initTrees();
    initPlayer();
    initCamera();
    initUI();
    initControls();

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

        // 设置玩家初始位置（地图中心）

        if (mapLayer_)

        {

            Size mapSize = mapLayer_->getMapSize();

            Vec2 centerPos = Vec2(mapSize.width / 2, mapSize.height / 2);

            // 检查中心位置是否可行走

            bool centerWalkable = mapLayer_->isWalkable(centerPos);

            CCLOG("Map center position: (%.1f, %.1f)", centerPos.x, centerPos.y);

            CCLOG("Map center walkable: %s", centerWalkable ? "YES" : "NO");

            if (!centerWalkable)

            {

                // 如果中心不可行走，寻找一个可行走的位置

                CCLOG("WARNING: Map center is blocked! Finding safe position...");

                Vec2 safePos = centerPos;

                bool foundSafe = false;

                // 在中心周围搜索可行走位置

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

                                CCLOG("Found safe position: (%.1f, %.1f)", safePos.x, safePos.y);

                            }

                        }

                    }

                }

                if (!foundSafe)

                {

                    // 如果还是找不到，使用一个固定的安全位置

                    safePos = Vec2(100, 100);

                    CCLOG("Using fallback position: (%.1f, %.1f)", safePos.x, safePos.y);

                }

                player_->setPosition(safePos);

            }

            else

            {

                player_->setPosition(centerPos);

            }

            CCLOG("Player position: (%.1f, %.1f)", player_->getPosition().x, player_->getPosition().y);

        }

        else

        {

            // 如果地图加载失败，放在屏幕中心

            auto visibleSize = Director::getInstance()->getVisibleSize();

            player_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));

            CCLOG("No map, player at screen center");

        }

        // 启用键盘控制

        player_->enableKeyboardControl();

        // 设置地图层引用（用于碰撞检测）

        if (mapLayer_)

        {

            player_->setMapLayer(mapLayer_);

            CCLOG("MapLayer reference passed to Player");

        }

        // 添加到场景（在地图层之上）

        this->addChild(player_, 10);

        CCLOG("Player added to scene");

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

    Vec2 origin = Director::getInstance()->getVisibleOrigin();

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
        "1-0: Select item | J: use item | K: water | ESC: Menu",
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
    itemLabel_ = Label::createWithSystemFont("Item: Hoe (1-0切换)", "Arial", 18);
    itemLabel_->setAnchorPoint(Vec2(0, 0.5f));
    itemLabel_->setPosition(Vec2(
        origin.x + 20,
        origin.y + 60
    ));
    itemLabel_->setColor(Color3B(200, 255, 200));
    uiLayer_->addChild(itemLabel_, 1);

    initToolbar();

    CCLOG("UI initialized - using simple fixed layer method");

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

    // 处理砍树计时

    updateChopping(delta);

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

    // ⭐⭐⭐ 关键：更新UI层位置，使其跟随摄像机（固定在屏幕上）

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

    // 如果有地图层，也显示瓦片坐标

    if (mapLayer_)

    {

        Vec2 tileCoord = mapLayer_->positionToTileCoord(playerPos);

        sprintf(posStr, "Position: (%.0f, %.0f) | Tile: (%.0f, %.0f)",

            playerPos.x, playerPos.y, tileCoord.x, tileCoord.y);

        positionLabel_->setString(posStr);

    }

}

void GameScene::handleFarmAction(bool waterOnly)

{

    if (!mapLayer_ || !farmManager_ || !player_)

        return;

    Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());

    FarmManager::ActionResult result{false, "Unavailable"};

    ItemType current = toolbarItems_.empty() ? ItemType::Hoe : toolbarItems_[selectedItemIndex_];

    if (waterOnly)

    {

        if (current == ItemType::WateringCan)

        {

            result = farmManager_->waterTile(tileCoord);

        }

        else
        {
            result = {false, "Need watering can to water"};
        }

    }

    else

    {

        switch (current)

        {

        case ItemType::Hoe:

            result = farmManager_->tillTile(tileCoord);

            break;

        case ItemType::WateringCan:

            result = farmManager_->waterTile(tileCoord);

            break;

        case ItemType::Scythe:

            result = farmManager_->harvestTile(tileCoord);

            break;

        case ItemType::Axe:

        {

            int idx = findTreeIndex(tileCoord);

            if (idx >= 0)

            {

                startChopTree(idx);

                result = {true, "Chopping tree... (2s)"};

            }

            else

            {

                result = {false, "No tree to chop here"};

            }

            break;

        }

        case ItemType::SeedTurnip:
        case ItemType::SeedPotato:
        case ItemType::SeedCorn:
        case ItemType::SeedTomato:
        case ItemType::SeedPumpkin:
        case ItemType::SeedBlueberry:

        {

            int cropId = getCropIdForItem(current);

            result = farmManager_->plantSeed(tileCoord, cropId);

            break;

        }

        default:

            result = {false, "Unknown item action"};

            break;

        }

    }

    Color3B color = result.success ? Color3B(120, 230, 140) : Color3B(255, 180, 120);

    showActionMessage(result.message, color);

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
    std::string name = getItemName(toolbarItems_[selectedItemIndex_]);

    if (itemLabel_)
    {
        itemLabel_->setString(StringUtils::format("Item: %s (1-0切换)", name.c_str()));
    }

    showActionMessage(StringUtils::format("切换到 %s", name.c_str()), Color3B(180, 220, 255));
}

std::string GameScene::getItemName(ItemType type) const
{
    switch (type)
    {
    case ItemType::Hoe: return "Hoe";
    case ItemType::WateringCan: return "Watering Can";
    case ItemType::Scythe: return "Scythe";
    case ItemType::Axe: return "Axe";
    case ItemType::SeedTurnip: return "Turnip Seed";
    case ItemType::SeedPotato: return "Potato Seed";
    case ItemType::SeedCorn: return "Corn Seed";
    case ItemType::SeedTomato: return "Tomato Seed";
    case ItemType::SeedPumpkin: return "Pumpkin Seed";
    case ItemType::SeedBlueberry: return "Blueberry Seed";
    default: return "Unknown";
    }
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

void GameScene::initTrees()
{
    trees_.clear();
    choppingIndex_ = -1;
    choppingTimer_ = 0.0f;

    if (!mapLayer_)
        return;

    // Place a few sample trees
    std::vector<Vec2> sampleTiles = { Vec2(8, 8), Vec2(10, 12), Vec2(12, 9) };

    Size tileSize = mapLayer_->getTileSize();
    float halfW = tileSize.width / 2.0f;
    float halfH = tileSize.height / 2.0f;

    for (const auto& t : sampleTiles)
    {
        Vec2 pos = mapLayer_->tileCoordToPosition(t);
        auto node = DrawNode::create();
        Vec2 bl(pos.x - halfW + 2, pos.y - halfH + 2);
        Vec2 tr(pos.x + halfW - 2, pos.y + halfH - 2);
        node->drawSolidRect(bl, tr, Color4F(0.2f, 0.6f, 0.2f, 0.9f));
        node->drawSolidCircle(pos, 10.0f, 0, 12, 1.0f, 1.0f, Color4F(0.1f, 0.5f, 0.1f, 0.9f));
        mapLayer_->addChild(node, 15);

        trees_.push_back(Tree{ t, node });
    }
}

int GameScene::findTreeIndex(const Vec2& tile) const
{
    for (int i = 0; i < static_cast<int>(trees_.size()); ++i)
    {
        if (trees_[i].tileCoord == tile)
            return i;
    }
    return -1;
}

void GameScene::startChopTree(int index)
{
    if (index < 0 || index >= static_cast<int>(trees_.size()))
        return;

    choppingIndex_ = index;
    choppingTimer_ = 2.0f; // 2s to chop
}

void GameScene::updateChopping(float delta)
{
    if (choppingIndex_ < 0 || choppingIndex_ >= static_cast<int>(trees_.size()))
        return;

    choppingTimer_ -= delta;
    if (choppingTimer_ <= 0.0f)
    {
        // Remove tree
        auto tree = trees_[choppingIndex_];
        if (tree.node)
        {
            tree.node->removeFromParent();
        }
        trees_.erase(trees_.begin() + choppingIndex_);
        choppingIndex_ = -1;
        choppingTimer_ = 0.0f;
        showActionMessage("Tree chopped!", Color3B(200, 255, 200));
    }
}
// ========== 返回菜单 ==========

void GameScene::backToMenu()

{

    CCLOG("Returning to menu...");

    auto scene = MenuScene::createScene();

    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, scene));

}


