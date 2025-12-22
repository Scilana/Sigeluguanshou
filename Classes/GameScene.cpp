#include "GameScene.h"
#include "MenuScene.h"
#include "FarmManager.h"
#include <cmath>
#include <queue>
#include <unordered_set>

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
        "1-0: Switch item | J: Use | K: Water | ESC: Menu",
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
    case EventKeyboard::KeyCode::KEY_MINUS: // Allow selecting item 10
        selectItemByIndex(10);
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
    // Snap to integral tile to avoid float drift
    tileCoord.x = std::round(tileCoord.x);
    tileCoord.y = std::round(tileCoord.y);

    FarmManager::ActionResult result{false, "Unavailable"};
    ItemType current = toolbarItems_.empty() ? ItemType::Hoe : toolbarItems_[selectedItemIndex_];

    if (waterOnly) {
        if (current == ItemType::WateringCan) {
            result = farmManager_->waterTile(tileCoord);
        } else {
            result = {false, "Need watering can to water"};
        }
    } else {
        switch (current) {
        case ItemType::Hoe:
            result = farmManager_->tillTile(tileCoord);
            break;
        case ItemType::WateringCan:
            result = farmManager_->waterTile(tileCoord);
            break;
        case ItemType::Scythe:
            result = farmManager_->harvestTile(tileCoord);
            break;
        case ItemType::Axe: {
            // 在当前及8方向查找带碰撞的格子，收集其连通块，只有小块（<=8格）且底图是树才认定为树
            cocos2d::Vec2 target = tileCoord;
            if (!findNearbyCollisionTile(tileCoord, target)) {
                result = {false, "No tree to chop here"};
                break;
            }

            std::vector<Vec2> component = collectCollisionComponent(target);
            const size_t kMaxTreeTiles = 8;
            if (component.empty() || component.size() > kMaxTreeTiles) {
                result = {false, "No tree to chop here"};
                break;
            }

            // 判断底图是否树：使用简单的 GID 白名单
            static const int treeGids[] = {43793, 43920, 43108, 43109, 43558, 43608, 44445, 43158};
            int baseGid = mapLayer_->getBaseTileGID(component.front());
            bool isTree = false;
            for (int gid : treeGids) {
                if (baseGid == gid) { isTree = true; break; }
            }
            if (!isTree) {
                result = {false, "No tree to chop here"};
                break;
            }

            PendingChop job;
            job.tiles = component;
            job.timer = 2.0f; // 2秒
            pendingChops_.push_back(job);
            result = {true, "Chopping tree... (2s)"};
            break;
        }
        case ItemType::SeedTurnip:
        case ItemType::SeedPotato:
        case ItemType::SeedCorn:
        case ItemType::SeedTomato:
        case ItemType::SeedPumpkin:
        case ItemType::SeedBlueberry: {
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

std::vector<Vec2> GameScene::collectCollisionComponent(const Vec2& start) const
{
    std::vector<Vec2> out;
    if (!mapLayer_ || !mapLayer_->hasCollisionAt(start))
        return out;

    Size mapSize = mapLayer_->getMapSizeInTiles();
    auto key = [](int x, int y) -> long long { return (static_cast<long long>(y) << 32) | (static_cast<unsigned long long>(x) & 0xffffffffULL); };

    std::queue<Vec2> q;
    std::unordered_set<long long> visited;
    q.push(start);
    visited.insert(key(static_cast<int>(start.x), static_cast<int>(start.y)));

    const Vec2 dirs[4] = { Vec2(1,0), Vec2(-1,0), Vec2(0,1), Vec2(0,-1) };
    while (!q.empty())
    {
        Vec2 t = q.front();
        q.pop();
        out.push_back(t);

        for (const auto& d : dirs)
        {
            int nx = static_cast<int>(t.x + d.x);
            int ny = static_cast<int>(t.y + d.y);
            if (nx < 0 || ny < 0 || nx >= mapSize.width || ny >= mapSize.height)
                continue;

            long long k = key(nx, ny);
            if (visited.count(k))
                continue;

            Vec2 nt(static_cast<float>(nx), static_cast<float>(ny));
            if (mapLayer_->hasCollisionAt(nt))
            {
                visited.insert(k);
                q.push(nt);
            }
        }
    }

    return out;
}

bool GameScene::findNearbyCollisionTile(const Vec2& centerTile, Vec2& outTile) const
{
    if (!mapLayer_)
        return false;

    const Vec2 offsets[9] = {
        Vec2(0,0), Vec2(1,0), Vec2(-1,0), Vec2(0,1), Vec2(0,-1),
        Vec2(1,1), Vec2(1,-1), Vec2(-1,1), Vec2(-1,-1)
    };

    for (const auto& off : offsets)
    {
        Vec2 candidate = centerTile + off;
        if (mapLayer_->hasCollisionAt(candidate))
        {
            outTile = candidate;
            return true;
        }
    }
    return false;
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
        ItemType::FishingRod, // ID 4 (index 4)
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
    std::string nameZh = getItemNameChinese(toolbarItems_[selectedItemIndex_]);

    if (itemLabel_)
    {
        itemLabel_->setString(StringUtils::format("Current item: %s (1-0 to switch)", nameZh.c_str()));
    }

    showActionMessage(StringUtils::format("Switched to %s", nameZh.c_str()), Color3B(180, 220, 255));
}

std::string GameScene::getItemName(ItemType type) const
{
    switch (type)
    {
    case ItemType::Hoe: return "Hoe";
    case ItemType::WateringCan: return "Watering Can";
    case ItemType::Scythe: return "Scythe";
    case ItemType::Axe: return "Axe";
    case ItemType::FishingRod: return "Fishing Rod";
    case ItemType::SeedTurnip: return "Turnip Seed";
    case ItemType::SeedPotato: return "Potato Seed";
    case ItemType::SeedCorn: return "Corn Seed";
    case ItemType::SeedTomato: return "Tomato Seed";
    case ItemType::SeedPumpkin: return "Pumpkin Seed";
    case ItemType::SeedBlueberry: return "Blueberry Seed";
    default: return "Unknown";
    }
}

std::string GameScene::getItemNameChinese(ItemType type) const
{
    // For simplicity, reuse English names to avoid encoding issues
    return getItemName(type);
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
    pendingChops_.clear();

    if (!mapLayer_)
        return;

    // Place a few sample trees (debug draw only)
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

void GameScene::updateChopping(float delta)
{
    if (pendingChops_.empty())
        return;

    // 逆序遍历以便安全删除
    for (int i = static_cast<int>(pendingChops_.size()) - 1; i >= 0; --i)
    {
        pendingChops_[i].timer -= delta;
        if (pendingChops_[i].timer <= 0.0f)
        {
            int fillGid = 0;
            for (const auto& t : pendingChops_[i].tiles)
            {
                static const Vec2 neigh[8] = { Vec2(1,0),Vec2(-1,0),Vec2(0,1),Vec2(0,-1),Vec2(1,1),Vec2(1,-1),Vec2(-1,1),Vec2(-1,-1) };
                for (const auto& n : neigh)
                {
                    Vec2 test = t + n;
                    if (!mapLayer_->hasCollisionAt(test))
                    {
                        int gid = mapLayer_->getBaseTileGID(test);
                        if (gid != 0) { fillGid = gid; break; }
                    }
                }
                if (fillGid != 0) break;
            }

            for (const auto& t : pendingChops_[i].tiles)
            {
                mapLayer_->clearCollisionAt(t);
                if (fillGid != 0)
                    mapLayer_->setBaseTileGID(t, fillGid);

                int idx = findTreeIndex(t);
                if (idx >= 0 && trees_[idx].node)
                {
                    trees_[idx].node->removeFromParent();
                    trees_.erase(trees_.begin() + idx);
                }
            }

            pendingChops_.erase(pendingChops_.begin() + i);
            showActionMessage("Tree chopped", Color3B(200, 255, 200));
        }
    }
}

// ==========================================
// Fishing Logic Restoration
// ==========================================

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

    auto scene = MenuScene::createScene();

    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, scene));

}



