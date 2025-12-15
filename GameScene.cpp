#include "GameScene.h"
#include "MenuScene.h"

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
    initMap();
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
    timeLabel_ = Label::createWithSystemFont("Day 1  Spring  10:00 AM", "Arial", 24);
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
        "WASD: Move  |  ESC: Back to Menu",
        "Arial", 18
    );
    hint->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + 30
    ));
    hint->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(hint, 1);

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
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        backToMenu();
    }
}

// ========== 更新函数 ==========

void GameScene::update(float delta)
{
    // 更新摄像机（跟随玩家）
    updateCamera();

    // 更新UI显示
    updateUI();
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

    // 如果有地图层，也显示瓦片坐标
    if (mapLayer_)
    {
        Vec2 tileCoord = mapLayer_->positionToTileCoord(playerPos);
        sprintf(posStr, "Position: (%.0f, %.0f) | Tile: (%.0f, %.0f)",
            playerPos.x, playerPos.y, tileCoord.x, tileCoord.y);
        positionLabel_->setString(posStr);
    }
}

// ========== 返回菜单 ==========

void GameScene::backToMenu()
{
    CCLOG("Returning to menu...");

    auto scene = MenuScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, scene));
}