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

    // 设置玩家初始位置（矿洞入口，暂时设为地图中心）
    Size mapSize = mineLayer_->getMapSizeInTiles();
    Size tileSize = mineLayer_->getTileSize();

    Vec2 startTileCoord(mapSize.width / 2, mapSize.height / 2);
    Vec2 startPosition = mineLayer_->tileCoordToPosition(startTileCoord);

    player_->setPosition(startPosition);
    player_->setMapLayer(mineLayer_);

    this->addChild(player_, 10);
    CCLOG("Player added to mine scene");
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
        "WASD: Move | ESC: Back to Farm",
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
    Vec2 cameraPos = camera->getPosition();

    // 平滑跟随
    float smoothing = 0.1f;
    Vec2 newCameraPos = cameraPos + (playerPos - cameraPos) * smoothing;

    camera->setPosition(newCameraPos);
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
