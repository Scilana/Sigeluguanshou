#include "MineScene.h"
#include "GameScene.h"
#include <cmath>

USING_NS_CC;

MineScene* MineScene::createScene(InventoryManager* inventory, int currentFloor)
{
    MineScene* ret = new (std::nothrow) MineScene();
    if (ret && ret->init(inventory, currentFloor))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool MineScene::init(InventoryManager* inventory, int currentFloor)
{
    if (!Scene::init())
        return false;

    CCLOG("========================================");
    CCLOG("Initializing Mine Scene (Floor %d)", currentFloor);
    CCLOG("========================================");

    // 初始化成员变量
    mineLayer_ = nullptr;
    player_ = nullptr;
    uiLayer_ = nullptr;
    inventory_ = inventory;
    currentFloor_ = currentFloor;

    if (!inventory_)
    {
        CCLOG("ERROR: MineScene requires a valid InventoryManager!");
        return false;
    }

    // 初始化各组件
    initMap();
    initPlayer();
    initCamera();
    initUI();
    initControls();

    // 启动更新
    this->scheduleUpdate();

    CCLOG("Mine Scene initialized successfully!");
    return true;
}

void MineScene::initMap()
{
    CCLOG("Initializing mine map...");

    // 根据楼层加载不同地图（使用新的 Mines 文件夹中的地图）
    std::string mapFile = StringUtils::format("map/Mines/%d.tmx", currentFloor_);

    mineLayer_ = MineLayer::create(mapFile);
    if (mineLayer_)
    {
        this->addChild(mineLayer_, 0);
        CCLOG("Mine layer added to scene (Floor %d)", currentFloor_);

        // 调整 Front 层的 Z-order，使其显示在玩家上方
        auto tmxMap = mineLayer_->getTMXMap();
        if (tmxMap)
        {
            CCLOG("TMX Map position: (%.2f, %.2f), Z-order: %d",
                  tmxMap->getPosition().x, tmxMap->getPosition().y,
                  tmxMap->getLocalZOrder());

            auto frontLayer = tmxMap->getLayer("Front");
            if (frontLayer)
            {
                CCLOG("✓ Front layer found!");
                CCLOG("  - Position: (%.2f, %.2f)", frontLayer->getPosition().x, frontLayer->getPosition().y);
                CCLOG("  - Visible: %s", frontLayer->isVisible() ? "YES" : "NO");
                CCLOG("  - Opacity: %d", frontLayer->getOpacity());
                CCLOG("  - Layer Size: (%.0f, %.0f)", frontLayer->getLayerSize().width, frontLayer->getLayerSize().height);
                CCLOG("  - Original Z-order: %d", frontLayer->getLocalZOrder());

                // 核心问题：
                // - mineLayer_ 在 scene 中 Z=0，包含 tmxMap
                // - player_ 在 scene 中 Z=10
                // - Front 层是 tmxMap 的子节点，所以它的渲染顺序受 tmxMap 的 Z-order 限制
                // - 即使设置 Front 层 Z=100，有效 Z 仍然是 0 + 100 = 100（在 mineLayer 范围内）
                // - 而 player_ 是场景的直接子节点，Z=10，会遮挡 mineLayer 中的所有内容

                // 解决方案：把 Front 层从 tmxMap 移到场景，设置更高的 Z-order
                Vec2 layerPosInMap = frontLayer->getPosition();
                Vec2 mapPosInMineLayer = tmxMap->getPosition();
                Vec2 mineLayerPosInScene = mineLayer_->getPosition();

                // 计算 Front 层在场景中的绝对位置
                Vec2 frontPosInScene = mineLayerPosInScene + mapPosInMineLayer + layerPosInMap;

                CCLOG("  - Calculating position:");
                CCLOG("    Front in map: (%.2f, %.2f)", layerPosInMap.x, layerPosInMap.y);
                CCLOG("    Map in mineLayer: (%.2f, %.2f)", mapPosInMineLayer.x, mapPosInMineLayer.y);
                CCLOG("    MineLayer in scene: (%.2f, %.2f)", mineLayerPosInScene.x, mineLayerPosInScene.y);
                CCLOG("    Front in scene: (%.2f, %.2f)", frontPosInScene.x, frontPosInScene.y);

                // 从 tmxMap 中移除 Front 层
                frontLayer->retain();
                tmxMap->removeChild(frontLayer, false);

                // 设置在场景中的绝对位置
                frontLayer->setPosition(frontPosInScene);

                // 添加到场景，Z-order 设为 20（高于玩家的 10）
                this->addChild(frontLayer, 20);
                frontLayer->release();

                CCLOG("✓ Front layer moved to scene:");
                CCLOG("  - New parent: Scene (not tmxMap)");
                CCLOG("  - Position in scene: (%.2f, %.2f)", frontPosInScene.x, frontPosInScene.y);
                CCLOG("  - Z-order in scene: 20 (player is 10)");
            }
            else
            {
                CCLOG("✗ WARNING: Front layer not found in TMX map");
            }
        }
        else
        {
            CCLOG("✗ WARNING: TMX map is null");
        }
    }
    else
    {
        CCLOG("ERROR: Failed to create mine layer!");
        CCLOG("Please ensure %s exists in Resources/", mapFile.c_str());
    }
}

void MineScene::initPlayer()
{
    CCLOG("Initializing player in mine...");

    if (!mineLayer_)
    {
        CCLOG("ERROR: Cannot initialize player without map!");
        return;
    }

    // 创建玩家
    player_ = Player::create();
    if (!player_)
    {
        CCLOG("ERROR: Failed to create player!");
        return;
    }

    // 设置玩家初始位置 - 寻找第一个可行走的位置
    Size mapSize = mineLayer_->getMapSizeInTiles();
    Size tileSize = mineLayer_->getTileSize();

    Vec2 startTileCoord(mapSize.width / 2, mapSize.height / 2);

    // 从地图中心开始螺旋搜索，寻找第一个可行走的位置
    bool foundWalkable = false;
    CCLOG("Searching for walkable position from center (%.0f, %.0f)...", mapSize.width / 2, mapSize.height / 2);

    for (int radius = 0; radius < 20 && !foundWalkable; radius++)
    {
        for (int dx = -radius; dx <= radius && !foundWalkable; dx++)
        {
            for (int dy = -radius; dy <= radius && !foundWalkable; dy++)
            {
                if (abs(dx) != radius && abs(dy) != radius)
                    continue; // 只检查当前圆周上的点

                Vec2 testCoord(mapSize.width / 2 + dx, mapSize.height / 2 + dy);

                // 检查是否在地图范围内且可行走
                if (testCoord.x >= 0 && testCoord.x < mapSize.width &&
                    testCoord.y >= 0 && testCoord.y < mapSize.height)
                {
                    if (!mineLayer_->hasCollisionAt(testCoord))
                    {
                        startTileCoord = testCoord;
                        foundWalkable = true;
                        CCLOG("✓ Found walkable position at tile (%.0f, %.0f), radius: %d",
                              testCoord.x, testCoord.y, radius);
                    }
                }
            }
        }
    }

    if (!foundWalkable)
    {
        CCLOG("✗ WARNING: Could not find walkable position within 20 tiles, using map center");
    }

    Vec2 startPosition = mineLayer_->tileCoordToPosition(startTileCoord);
    player_->setPosition(startPosition);
    player_->setMapLayer(mineLayer_);
    player_->enableKeyboardControl();  // 启用键盘控制

    this->addChild(player_, 10);
    CCLOG("✓ Player added to mine scene at position (%.2f, %.2f)", startPosition.x, startPosition.y);
}

void MineScene::initCamera()
{
    CCLOG("Initializing camera...");

    if (!player_)
        return;

    // 摄像机跟随玩家
    auto camera = this->getDefaultCamera();
    if (camera)
    {
        camera->setPosition(player_->getPosition());
        CCLOG("Camera initialized and following player");
    }
}

void MineScene::initUI()
{
    CCLOG("Initializing UI...");

    uiLayer_ = Layer::create();
    if (!uiLayer_)
    {
        CCLOG("ERROR: Failed to create UI layer!");
        return;
    }

    this->addChild(uiLayer_, 100);

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 矿洞层数显示
    floorLabel_ = Label::createWithSystemFont(
        StringUtils::format("Mine Floor %d", currentFloor_),
        "Arial", 24
    );
    floorLabel_->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height - 30
    ));
    floorLabel_->setColor(Color3B(255, 215, 0)); // 金色
    uiLayer_->addChild(floorLabel_, 1);

    // 位置显示（调试用）
    positionLabel_ = Label::createWithSystemFont(
        "Position: (0, 0)",
        "Arial", 18
    );
    positionLabel_->setPosition(Vec2(
        origin.x + 100,
        origin.y + visibleSize.height - 60
    ));
    positionLabel_->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(positionLabel_, 1);

    // 操作提示
    auto hint = Label::createWithSystemFont(
        "WASD: Move | Q: Previous Floor | E: Next Floor | ESC: Back to Farm",
        "Arial", 18
    );
    hint->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + 30
    ));
    hint->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(hint, 1);

    CCLOG("UI initialized");
}

void MineScene::initControls()
{
    CCLOG("Initializing controls...");

    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(MineScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    CCLOG("Controls initialized");
}

void MineScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_ESCAPE:
        backToFarm();
        break;
    case EventKeyboard::KeyCode::KEY_Q:
        goToPreviousFloor();
        break;
    case EventKeyboard::KeyCode::KEY_E:
        goToNextFloor();
        break;
    default:
        break;
    }
}

void MineScene::update(float delta)
{
    Scene::update(delta);
    updateCamera();
    updateUI();
}

void MineScene::updateCamera()
{
    if (!player_)
        return;

    auto camera = this->getDefaultCamera();
    if (!camera)
        return;

    Vec2 playerPos = player_->getPosition();

    // 直接设置摄像机位置，不使用平滑跟随
    // 这样可以避免场景切换时的抖动
    camera->setPosition(playerPos);
}

void MineScene::updateUI()
{
    if (!player_)
        return;

    // 更新位置显示
    Vec2 playerPos = player_->getPosition();
    Vec2 tileCoord = mineLayer_->positionToTileCoord(playerPos);

    positionLabel_->setString(
        StringUtils::format("Pos: (%.0f, %.0f) Tile: (%.0f, %.0f)",
            playerPos.x, playerPos.y,
            tileCoord.x, tileCoord.y)
    );
}

void MineScene::backToFarm()
{
    CCLOG("Returning to farm...");

    // 创建农场场景并切换
    auto farmScene = GameScene::createScene();
    auto transition = TransitionFade::create(1.0f, farmScene);
    Director::getInstance()->replaceScene(transition);
}

void MineScene::goToPreviousFloor()
{
    if (currentFloor_ <= 1)
    {
        CCLOG("Already at first floor!");
        return;
    }

    CCLOG("Going to previous floor: %d -> %d", currentFloor_, currentFloor_ - 1);

    // 创建上一层矿洞场景
    auto prevFloorScene = MineScene::createScene(inventory_, currentFloor_ - 1);
    auto transition = TransitionFade::create(0.5f, prevFloorScene);
    Director::getInstance()->replaceScene(transition);
}

void MineScene::goToNextFloor()
{
    // 最多 120 层（根据您的地图文件）
    if (currentFloor_ >= 120)
    {
        CCLOG("Already at last floor!");
        return;
    }

    CCLOG("Going to next floor: %d -> %d", currentFloor_, currentFloor_ + 1);

    // 创建下一层矿洞场景
    auto nextFloorScene = MineScene::createScene(inventory_, currentFloor_ + 1);
    auto transition = TransitionFade::create(0.5f, nextFloorScene);
    Director::getInstance()->replaceScene(transition);
}
