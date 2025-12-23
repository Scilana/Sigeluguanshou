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
    else
    {
        delete ret;
        ret = nullptr;
        return nullptr;
    }
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
    miningManager_ = nullptr;
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
    initMining();
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

    // 根据楼层加载不同地图
    // 你可以根据楼层数动态选择地图文件
    std::string mapFile = "map/mine_floor1.tmx";  // 默认地图

    // 如果有多个楼层的地图文件
    // mapFile = StringUtils::format("map/mine_floor%d.tmx", currentFloor_);

    mineLayer_ = MineLayer::create(mapFile);
    if (mineLayer_)
    {
        this->addChild(mineLayer_, 0);
        CCLOG("Mine layer added to scene");
    }
    else
    {
        CCLOG("ERROR: Failed to create mine layer!");
        CCLOG("Please ensure %s exists in Resources/map/", mapFile.c_str());
    }
}

void MineScene::initMining()
{
    if (!mineLayer_ || !inventory_)
    {
        CCLOG("Cannot initialize mining: missing mineLayer or inventory");
        return;
    }

    miningManager_ = MiningManager::create(mineLayer_, inventory_);
    if (miningManager_)
    {
        this->addChild(miningManager_, 5);
        CCLOG("MiningManager initialized");
    }
    else
    {
        CCLOG("Failed to create MiningManager");
    }
}

void MineScene::initPlayer()
{
    CCLOG("Initializing player in mine...");

    player_ = Player::create();
    if (player_)
    {
        // 设置玩家初始位置（矿洞入口）
        if (mineLayer_)
        {
            Size mapSize = mineLayer_->getMapSize();
            Vec2 startPos = Vec2(mapSize.width / 2, mapSize.height - 100);

            // 查找安全位置
            if (!mineLayer_->isWalkable(startPos))
            {
                CCLOG("Start position blocked, finding safe position...");
                bool foundSafe = false;
                for (int radius = 1; radius < 10 && !foundSafe; radius++)
                {
                    for (int dx = -radius; dx <= radius && !foundSafe; dx++)
                    {
                        for (int dy = -radius; dy <= radius && !foundSafe; dy++)
                        {
                            Vec2 testPos = startPos + Vec2(dx * 32, dy * 32);
                            if (mineLayer_->isWalkable(testPos))
                            {
                                startPos = testPos;
                                foundSafe = true;
                            }
                        }
                    }
                }
            }

            player_->setPosition(startPos);
            CCLOG("Player position: (%.1f, %.1f)", startPos.x, startPos.y);
        }
        else
        {
            auto visibleSize = Director::getInstance()->getVisibleSize();
            player_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
        }

        // 启用键盘控制
        player_->enableKeyboardControl();

        // 设置地图层引用
        if (mineLayer_)
        {
            player_->setMapLayer(mineLayer_);
        }

        // 添加到场景
        this->addChild(player_, 10);
        CCLOG("Player added to mine scene");
    }
    else
    {
        CCLOG("ERROR: Failed to create player!");
    }
}

void MineScene::initCamera()
{
    CCLOG("Initializing camera...");

    auto camera = this->getDefaultCamera();
    if (camera && player_)
    {
        Vec2 playerPos = player_->getPosition();
        camera->setPosition(playerPos.x, playerPos.y);
        CCLOG("Camera initialized at player position");
    }
}

void MineScene::initUI()
{
    CCLOG("Initializing UI...");

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 创建UI层
    uiLayer_ = Layer::create();
    uiLayer_->setGlobalZOrder(1000);
    this->addChild(uiLayer_, 1000);

    // 顶部信息栏背景
    auto topBar = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, 60);
    topBar->setAnchorPoint(Vec2(0, 1));
    topBar->setPosition(Vec2(origin.x, origin.y + visibleSize.height));
    uiLayer_->addChild(topBar, 0);

    // 矿洞层数显示
    floorLabel_ = Label::createWithSystemFont(
        StringUtils::format("Mine - Floor %d", currentFloor_),
        "Arial", 24
    );
    floorLabel_->setAnchorPoint(Vec2(0, 0.5));
    floorLabel_->setPosition(Vec2(origin.x + 20, origin.y + visibleSize.height - 30));
    floorLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(floorLabel_, 1);

    // 体力显示（右上角）
    healthLabel_ = Label::createWithSystemFont("HP: 100/100", "Arial", 24);
    healthLabel_->setAnchorPoint(Vec2(1, 0.5));
    healthLabel_->setPosition(Vec2(
        origin.x + visibleSize.width - 20,
        origin.y + visibleSize.height - 30
    ));
    healthLabel_->setColor(Color3B(100, 255, 100));
    uiLayer_->addChild(healthLabel_, 1);

    // 位置显示（调试）
    positionLabel_ = Label::createWithSystemFont("Position: (0, 0)", "Arial", 18);
    positionLabel_->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height - 70
    ));
    positionLabel_->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(positionLabel_, 1);

    // 操作提示
    auto hint = Label::createWithSystemFont(
        "J: Mine | B: Inventory | Enter: Stairs | ESC: Back to Farm",
        "Arial", 18
    );
    hint->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + 30
    ));
    hint->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(hint, 1);

    // 操作提示标签
    actionLabel_ = Label::createWithSystemFont("", "Arial", 20);
    actionLabel_->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + 60
    ));
    actionLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(actionLabel_, 1);

    // 当前工具显示
    itemLabel_ = Label::createWithSystemFont("Current tool: Pickaxe", "Arial", 18);
    itemLabel_->setAnchorPoint(Vec2(0, 0.5f));
    itemLabel_->setPosition(Vec2(origin.x + 20, origin.y + 60));
    itemLabel_->setColor(Color3B(200, 255, 200));
    uiLayer_->addChild(itemLabel_, 1);

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
    case EventKeyboard::KeyCode::KEY_J:
        handleMiningAction();
        break;
    case EventKeyboard::KeyCode::KEY_ENTER:
    case EventKeyboard::KeyCode::KEY_KP_ENTER:
        if (isPlayerOnStairs())
        {
            goToNextFloor();
        }
        else
        {
            showActionMessage("Find the stairs to go deeper!", Color3B(255, 180, 120));
        }
        break;
    default:
        break;
    }
}

void MineScene::update(float delta)
{
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
    Vec3 currentPos = camera->getPosition3D();
    Vec3 targetPos = Vec3(playerPos.x, playerPos.y, currentPos.z);

    float smoothFactor = 0.1f;
    Vec3 newPos = currentPos + (targetPos - currentPos) * smoothFactor;

    // 限制摄像机范围
    if (mineLayer_)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        Size mapSize = mineLayer_->getMapSize();

        float minX = visibleSize.width / 2;
        float maxX = mapSize.width - visibleSize.width / 2;
        float minY = visibleSize.height / 2;
        float maxY = mapSize.height - visibleSize.height / 2;

        if (mapSize.width < visibleSize.width)
        {
            minX = maxX = mapSize.width / 2;
        }
        if (mapSize.height < visibleSize.height)
        {
            minY = maxY = mapSize.height / 2;
        }

        newPos.x = std::max(minX, std::min(maxX, newPos.x));
        newPos.y = std::max(minY, std::min(maxY, newPos.y));
    }

    camera->setPosition3D(newPos);

    // 更新UI层位置
    if (uiLayer_)
    {
        Vec3 cameraPos = camera->getPosition3D();
        auto visibleSize = Director::getInstance()->getVisibleSize();
        Vec2 uiPos = Vec2(
            cameraPos.x - visibleSize.width / 2,
            cameraPos.y - visibleSize.height / 2
        );
        uiLayer_->setPosition(uiPos);
    }
}

void MineScene::updateUI()
{
    if (!player_)
        return;

    // 更新位置显示
    Vec2 playerPos = player_->getPosition();
    char posStr[64];
    sprintf(posStr, "Position: (%.0f, %.0f)", playerPos.x, playerPos.y);
    positionLabel_->setString(posStr);

    if (mineLayer_)
    {
        Vec2 tileCoord = mineLayer_->positionToTileCoord(playerPos);
        sprintf(posStr, "Position: (%.0f, %.0f) | Tile: (%.0f, %.0f)",
            playerPos.x, playerPos.y, tileCoord.x, tileCoord.y);
        positionLabel_->setString(posStr);
    }

    // 更新体力（如果你实现了体力系统）
    // healthLabel_->setString(StringUtils::format("HP: %d/100", currentHealth));
}

void MineScene::handleMiningAction()
{
    if (!mineLayer_ || !miningManager_ || !player_)
        return;

    Vec2 tileCoord = mineLayer_->positionToTileCoord(player_->getPosition());
    tileCoord.x = std::round(tileCoord.x);
    tileCoord.y = std::round(tileCoord.y);

    auto result = miningManager_->mineTile(tileCoord);
    Color3B color = result.success ? Color3B(120, 230, 140) : Color3B(255, 180, 120);
    showActionMessage(result.message, color);
}

void MineScene::showActionMessage(const std::string& text, const Color3B& color)
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
        nullptr
    );
    actionLabel_->runAction(seq);
}

void MineScene::backToFarm()
{
    CCLOG("Returning to farm...");

    // 这里需要切换回 GameScene
    // 注意：需要保持 inventory_ 的引用
    // auto scene = GameScene::createScene();
    // Director::getInstance()->replaceScene(TransitionFade::create(1.0f, scene));

    showActionMessage("Exiting mine... (Not implemented yet)", Color3B::YELLOW);
}

void MineScene::goToNextFloor()
{
    CCLOG("Going to floor %d", currentFloor_ + 1);

    // 创建下一层场景
    auto nextFloor = MineScene::createScene(inventory_, currentFloor_ + 1);
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, nextFloor));
}

bool MineScene::isPlayerOnStairs() const
{
    if (!mineLayer_ || !player_)
        return false;

    Vec2 tileCoord = mineLayer_->positionToTileCoord(player_->getPosition());
    tileCoord.x = std::round(tileCoord.x);
    tileCoord.y = std::round(tileCoord.y);

    return mineLayer_->isStairsAt(tileCoord);
}
