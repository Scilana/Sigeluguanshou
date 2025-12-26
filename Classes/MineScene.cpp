#include "MineScene.h"
#include "MineLayer.h"
#include "Player.h"
#include "InventoryManager.h"
#include "InventoryUI.h"
#include "MiningManager.h"
#include "Monster.h"
#include "TreasureChest.h"
#include "Weapon.h"
#include "GameScene.h"
#include "Slime.h"
#include "Zombie.h"
#include <algorithm>
#include "EnergyBar.h"
#include "HouseScene.h"
#include "TimeManager.h"


USING_NS_CC;

namespace
{
    const float kToolbarSlotSize = 48.0f;
    const float kToolbarSlotPadding = 6.0f;
    const float kToolbarIconPadding = 6.0f;
    const Color4B kToolbarBarColor(40, 35, 30, 220);
    const Color3B kToolbarSlotColor(70, 60, 50);
    const Color3B kToolbarSlotSelectedColor(170, 150, 95);

    Sprite* createToolbarIcon(ItemType itemType)
    {
        std::string path = InventoryManager::getItemIconPath(itemType);
        if (path.empty() || !FileUtils::getInstance()->isFileExist(path)) {
            return nullptr;
        }

        auto icon = Sprite::create(path);
        if (!icon) {
            return nullptr;
        }

        auto size = icon->getContentSize();
        float maxSize = kToolbarSlotSize - kToolbarIconPadding * 2.0f;
        float scale = std::min(maxSize / size.width, maxSize / size.height);
        icon->setScale(scale);
        return icon;
    }
}

// 定义静态成员
std::map<int, int> MineScene::openedChestsPerWeek_;

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

    inventory_ = inventory;
    currentFloor_ = currentFloor;

    CCLOG("========================================");
    CCLOG("Initializing Mine Scene (Floor %d)", currentFloor);
    CCLOG("========================================");

    // 初始化成员变量
    mineLayer_ = nullptr;
    player_ = nullptr;
    uiLayer_ = nullptr;
    inventory_ = inventory;
    miningManager_ = nullptr;
    currentFloor_ = currentFloor;
    monsterSpawnTimer_ = 0.0f;
    currentWeapon_ = ItemType::ITEM_NONE;
    attackCooldown_ = 0.5f;
    currentAttackCooldown_ = 0.0f;

    if (!inventory_)
    {
        CCLOG("ERROR: MineScene requires a valid InventoryManager!");
        return false;
    }

    // 初始化各组件
    initMap();
    if (mineLayer_)
    {
        miningManager_ = MiningManager::create(mineLayer_, inventory_);
        if (miningManager_)
        {
            this->addChild(miningManager_, 0);
        }
    }
    initPlayer();
    initCamera();
    initUI();
    initControls();
    initMonsters();
    initChests();
    initElevator(); // [New]
    initWishingWell();
    initToolbar();
    initToolbarUI(); // 初始化工具栏

    // 启动更新
    this->scheduleUpdate();

    CCLOG("Mine Scene initialized successfully!");
    return true;
}

void MineScene::initMap()
{
    CCLOG("Initializing mine map...");

    // 使用新的 Mines 文件夹中的地图
    std::string mapFile = StringUtils::format("map/Mines/%d.tmx", currentFloor_);

    mineLayer_ = MineLayer::create(mapFile);
    if (mineLayer_)
    {
        this->addChild(mineLayer_, 0);
        CCLOG("Mine layer added to scene (Floor %d)", currentFloor_);

        // 处理所有图层的 Z-order，确保正确的渲染顺序
        // 渲染顺序（从下到上）：Front -> Back -> Buildings -> 玩家(Z=10) -> mine1
        auto tmxMap = mineLayer_->getTMXMap();
        if (tmxMap)
        {
            Size mapSize = tmxMap->getMapSize();
            Size tileSize = tmxMap->getTileSize();
            CCLOG("TMX Map info:");
            CCLOG("  - Position: (%.2f, %.2f)", tmxMap->getPosition().x, tmxMap->getPosition().y);
            CCLOG("  - Z-order: %d", tmxMap->getLocalZOrder());
            CCLOG("  - Map size: %.0f x %.0f tiles", mapSize.width, mapSize.height);
            CCLOG("  - Tile size: %.0f x %.0f px", tileSize.width, tileSize.height);
            CCLOG("  - Total size: %.0f x %.0f px", mapSize.width * tileSize.width, mapSize.height * tileSize.height);

            // 打印所有图层名称
            CCLOG("--- Layer List ---");
            for (const auto& child : tmxMap->getChildren())
            {
                auto layer = dynamic_cast<TMXLayer*>(child);
                if (layer)
                {
                    CCLOG("Layer Name: %s, Z: %d, Visible: %d", layer->getLayerName().c_str(), layer->getLocalZOrder(), layer->isVisible());
                }
            }
            CCLOG("------------------");

            // 处理 Front 层 (通常是遮挡层/顶部墙壁，放在最上层)
            auto frontLayer = tmxMap->getLayer("Front");
            if (frontLayer)
            {
                CCLOG("âœ“ Front layer found!");
                // Front 通常是在玩家之上的遮挡层 (Roof/TreeTop)
                // 这里设置为 20 (Player is 10)
                frontLayer->setLocalZOrder(20);
                frontLayer->setVisible(true);
                frontLayer->setOpacity(255);
                CCLOG("âœ“ Front layer set to Z-order 20 (Above Player)");
            }
            else
            {
                CCLOG("âœ— WARNING: Front layer not found in TMX map");
            }

            // 处理 Back 层（背景，在 Front 上面）
            // 注：这里保留原注释逻辑，尽管通常 Back 在最底层，但代码逻辑未变
            auto backLayer = tmxMap->getLayer("Back");
            if (backLayer)
            {
                CCLOG("âœ“ Back layer found - keeping in tmxMap");
                CCLOG("  - Original opacity: %d", backLayer->getOpacity());
                CCLOG("  - Layer size: %.0f x %.0f tiles", backLayer->getLayerSize().width, backLayer->getLayerSize().height);

                // 采样一些瓦片来检查是否有数据
                int sampleCount = 0;
                int nonZeroCount = 0;
                for (int y = 0; y < backLayer->getLayerSize().height && y < 5; y++)
                {
                    for (int x = 0; x < backLayer->getLayerSize().width && x < 10; x++)
                    {
                        int gid = backLayer->getTileGIDAt(Vec2(x, y));
                        sampleCount++;
                        if (gid != 0)
                        {
                            nonZeroCount++;
                            if (nonZeroCount <= 3)  // 只打印前3个
                            {
                                CCLOG("    Sample tile at (%d,%d): GID=%d", x, y, gid);
                            }
                        }
                    }
                }
                CCLOG("  - Tile sampling: %d/%d tiles are non-zero in top-left area", nonZeroCount, sampleCount);

                backLayer->setLocalZOrder(-100);  // 在 Front 上面
                backLayer->setVisible(true);
                backLayer->setOpacity(255);  // 确保完全不透明

                CCLOG("  - Back layer visible: %s, opacity: %d",
                    backLayer->isVisible() ? "YES" : "NO",
                    backLayer->getOpacity());
            }

            // 处理 Buildings 层（墙壁/碰撞层，在 Back 上面）
            auto buildingsLayer = tmxMap->getLayer("Buildings");
            if (buildingsLayer)
            {
                CCLOG("âœ“ Buildings layer found!");
                CCLOG("  - Visible: %s", buildingsLayer->isVisible() ? "YES" : "NO");
                CCLOG("  - Original opacity: %d", buildingsLayer->getOpacity());
                CCLOG("  - Layer Size: (%.0f, %.0f)", buildingsLayer->getLayerSize().width, buildingsLayer->getLayerSize().height);

                buildingsLayer->setLocalZOrder(-50);  // 在 Back 上面
                buildingsLayer->setVisible(true);  // 确保可见
                buildingsLayer->setOpacity(255);  // 确保完全不透明
                CCLOG("âœ“ Buildings layer set to Z-order -50, forced visible, opacity: 255");
            }
            else
            {
                CCLOG("âœ— WARNING: Buildings layer not found in TMX map");
            }

            // 处理 mine1 层（矿石贴图，放在玩家下方，让玩家能遮住矿石）
            auto mine1Layer = tmxMap->getLayer("mine1");
            if (mine1Layer)
            {
                CCLOG("âœ“ mine1 layer found!");
                CCLOG("  - Position: (%.2f, %.2f)", mine1Layer->getPosition().x, mine1Layer->getPosition().y);
                CCLOG("  - Visible: %s", mine1Layer->isVisible() ? "YES" : "NO");
                CCLOG("  - Original opacity: %d", mine1Layer->getOpacity());
                CCLOG("  - Layer Size: (%.0f, %.0f)", mine1Layer->getLayerSize().width, mine1Layer->getLayerSize().height);

                // mine1 层保留在 tmxMap 中，设置在 Buildings 和玩家之间
                mine1Layer->setLocalZOrder(-25);  // 在 Buildings(-50) 上面，但在玩家(10)下面
                mine1Layer->setVisible(true);
                mine1Layer->setOpacity(255);  // 确保完全不透明
                CCLOG("âœ“ mine1 layer set to Z-order -25 (above Buildings, below Player), opacity: 255");
            }
            else
            {
                CCLOG("âœ— WARNING: mine1 layer not found in TMX map");
            }
        }
        else
        {
            CCLOG("âœ— WARNING: TMX map is null");
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
    if (player_)
    {
        CCLOG("Initializing player in mine...");

        if (mineLayer_)
        {
            // 获取地图大小（像素）
            Size mapSize = mineLayer_->getMapSize();
            CCLOG("Map size: (%.2f, %.2f) pixels", mapSize.width, mapSize.height);

            // 计算地图中心点（像素坐标）
            Vec2 mapCenter = Vec2(mapSize.width / 2, mapSize.height / 2);
            CCLOG("Map center: (%.2f, %.2f)", mapCenter.x, mapCenter.y);

            // 从中心开始螺旋搜索，寻找第一个可行走的位置
            Vec2 startPos = mapCenter;
            bool foundWalkable = false;

            // 先检查中心点是否可行走
            if (mineLayer_->isWalkable(mapCenter))
            {
                foundWalkable = true;
                startPos = mapCenter;
                CCLOG("âœ“ Center position is walkable");
            }
            else
            {
                CCLOG("Center position not walkable, searching nearby...");

                // 螺旋搜索半径（像素）
                for (int radius = 16; radius < 320 && !foundWalkable; radius += 16)
                {
                    // 每个半径检查 8 个方向
                    for (int angle = 0; angle < 360 && !foundWalkable; angle += 45)
                    {
                        float rad = angle * M_PI / 180.0f;
                        Vec2 testPos = mapCenter + Vec2(cos(rad) * radius, sin(rad) * radius);

                        // 检查是否在地图范围内
                        if (testPos.x >= 0 && testPos.x < mapSize.width &&
                            testPos.y >= 0 && testPos.y < mapSize.height)
                        {
                            if (mineLayer_->isWalkable(testPos))
                            {
                                startPos = testPos;
                                foundWalkable = true;
                                CCLOG("âœ“ Found walkable position at (%.2f, %.2f), radius: %d",
                                    testPos.x, testPos.y, radius);
                            }
                        }
                    }
                }
            }

            if (!foundWalkable)
            {
                CCLOG("âœ— WARNING: Could not find walkable position, trying random...");
                startPos = getRandomWalkablePosition();
            }

            player_->setPosition(startPos);
            CCLOG("âœ“ Player positioned at (%.2f, %.2f)", startPos.x, startPos.y);
        }
        else
        {
            auto visibleSize = Director::getInstance()->getVisibleSize();
            player_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
            CCLOG("âœ— WARNING: No map layer, using screen center");
        }

        player_->enableKeyboardControl();
        if (mineLayer_)
        {
            player_->setMapLayer(mineLayer_);
        }

        this->addChild(player_, 10);
        CCLOG("âœ“ Player added to scene");
    }
    else
    {
        CCLOG("ERROR: Failed to create player!");
    }
}

void MineScene::initCamera()
{
    auto camera = this->getDefaultCamera();
    if (camera && player_)
    {
        Vec2 playerPos = player_->getPosition();
        camera->setPosition(playerPos.x, playerPos.y);
    }
    else if (!player_)
    {
        CCLOG("ERROR: Cannot init camera without player");
    }
}

void MineScene::initUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    uiLayer_ = Layer::create();
    uiLayer_->setGlobalZOrder(1000);
    this->addChild(uiLayer_, 1000);

    // 顶部状态栏
    auto topBar = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, 40);
    topBar->setAnchorPoint(Vec2(0, 1));
    topBar->setPosition(Vec2(origin.x, origin.y + visibleSize.height));
    uiLayer_->addChild(topBar, 0);

    // 层数显示
    std::string floorStr = StringUtils::format("Floor: %d", currentFloor_);
    floorLabel_ = Label::createWithSystemFont(floorStr, "Arial", 20);
    floorLabel_->setAnchorPoint(Vec2(0, 0.5));
    floorLabel_->setPosition(Vec2(origin.x + 20, origin.y + visibleSize.height - 20));
    floorLabel_->setColor(Color3B::YELLOW);
    uiLayer_->addChild(floorLabel_, 1);
    
    // Time Label in Mine
    auto tm = TimeManager::getInstance();
    std::string timeStr = "Day ?, ??:??";
    if (tm) {
         timeStr = StringUtils::format("Day %d, %02d:%02d", tm->getDay(), tm->getHour(), tm->getMinute());
    }
    auto timeLabel = Label::createWithSystemFont(timeStr, "Arial", 20);
    timeLabel->setName("TimeLabel"); // Give it a name to find update later
    timeLabel->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height - 20));
    timeLabel->setColor(Color3B::WHITE);
    uiLayer_->addChild(timeLabel, 1);


    // 当前物品
    itemLabel_ = Label::createWithSystemFont("Tool: None", "Arial", 18);
    itemLabel_->setAnchorPoint(Vec2(1, 0.5));
    itemLabel_->setPosition(Vec2(origin.x + visibleSize.width - 20, origin.y + visibleSize.height - 20));
    itemLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(itemLabel_, 1);

    // 添加操作提示
    auto tipLabel = Label::createWithSystemFont("(Keys 1-8 to switch)", "Arial", 12);
    tipLabel->setAnchorPoint(Vec2(1, 0.5));
    tipLabel->setPosition(Vec2(origin.x + visibleSize.width - 20, origin.y + visibleSize.height - 40));
    tipLabel->setColor(Color3B::GRAY);
    uiLayer_->addChild(tipLabel, 1);

    // 操作提示
    actionLabel_ = Label::createWithSystemFont("", "Arial", 24);
    actionLabel_->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 100));
    actionLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(actionLabel_, 1);

    // 帮助提示
    auto helpLabel = Label::createWithSystemFont(
        "WASD: Move | J: Attack/Mine | M: Elevator | ENTER: Stairs", "Arial", 14);
    helpLabel->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 20));
    helpLabel->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(helpLabel, 1);

    // 能量条
    if (player_)
    {
        auto energyBar = EnergyBar::create(player_);
        if (energyBar)
        {
            energyBar->setName("EnergyBar");
            this->addChild(energyBar, 100);
        }
    }
}

void MineScene::initControls()
{
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(MineScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
}

void MineScene::initMonsters()
{
    monsters_.clear();

    // 怪物太多？减少到每层最多只有两只
    int initialCount = 1 + (rand() % 2); // 1 or 2
    if (currentFloor_ > 5) initialCount = 2; // 后期固定2只

    for (int i = 0; i < initialCount; ++i)
    {
        spawnMonster();
    }
}
void MineScene::initElevator()
{
    if (!mineLayer_) return;

    auto map = mineLayer_->getTMXMap();
    if (!map) return;

    Size mapSize = map->getContentSize();
    Vec2 centerPos = Vec2(mapSize.width / 2, mapSize.height / 2);

    // Create Elevator Sprite
    elevatorSprite_ = Sprite::create("myhouse/elevator.png"); // Try house asset first
    if (!elevatorSprite_) {
        // Fallback: Create a visual placeholder if file missing
        auto draw = DrawNode::create();
        draw->drawSolidRect(Vec2(-20, -20), Vec2(20, 20), Color4F(0.4f, 0.4f, 0.5f, 1.0f)); // Greyish
        elevatorSprite_ = Sprite::create();
        elevatorSprite_->addChild(draw);
        elevatorSprite_->setContentSize(Size(40, 40));
    }

    elevatorSprite_->setPosition(centerPos);
    this->addChild(elevatorSprite_, 5);
}

void MineScene::initChests()
{
    chests_.clear();

    // 宝箱数量：至多一个，甚至不刷
    // 设定 40% 的概率出现一个宝箱
    int chestCount = (rand() % 100 < 40) ? 1 : 0;

    if (chestCount > 0)
    {
        // 1. 检查这一周这一层的宝箱是否已经开过
        int dayCount = 1;
        if (TimeManager::getInstance()) dayCount = TimeManager::getInstance()->getDay();
        int currentWeek = (dayCount - 1) / 7 + 1;


        if (openedChestsPerWeek_.count(currentFloor_) > 0 &&
            openedChestsPerWeek_[currentFloor_] == currentWeek)
        {
            // 这一周已经开过了，直接不生成，避免玩家重复进出刷宝箱或看到已开箱子
            return;
        }

        // 2. 生成宝箱
        auto chest = TreasureChest::create(currentFloor_);
        if (chest)
        {
            chest->setPosition(getRandomWalkablePosition());
            this->addChild(chest, 5);
            chests_.push_back(chest);
        }
    }
}

void MineScene::update(float delta)
{
    Scene::update(delta);
    updateCamera();
    updateUI();
    // updateChopping removed as it belongs in GameScene

    // 更新能量条位置（始终在右下角，跟随摄像机）
    auto energyBar = this->getChildByName("EnergyBar");
    auto camera = this->getDefaultCamera();
    if (energyBar && camera)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        energyBar->setPosition(Vec2(camera->getPositionX() + visibleSize.width / 2 - 50,
            camera->getPositionY() - visibleSize.height / 2 + 110));
    }
    updateMonsters(delta);

    // 1. 模拟时间流逝
    auto tm = TimeManager::getInstance();
    if (tm) {
        tm->update(delta);
        
        // Update Time Label
        if (uiLayer_) {
            auto label = dynamic_cast<Label*>(uiLayer_->getChildByName("TimeLabel"));
            if (label) {
                label->setString(StringUtils::format("Day %d, %02d:%02d", tm->getDay(), tm->getHour(), tm->getMinute()));
            }
        }

        // 2. 检查是否到达午夜
        if (tm->isMidnight())
        {
            CCLOG("It's midnight! Passing out...");
            if (inventory_) inventory_->removeMoney(200);
            showActionMessage("Passed out...", Color3B::RED);
            Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HouseScene::createScene(true)));
            return;
        }
    }


    // 3. 检查生命值 (死亡逻辑)
    if (player_ && player_->getHp() <= 0)
    {
        CCLOG("Player died in mine!");
        if (inventory_) inventory_->removeMoney(200);
        showActionMessage("You died...", Color3B::RED);
        Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HouseScene::createScene(true)));
        return;
    }

    // 攻击冷却
    if (currentAttackCooldown_ > 0)
    {
        currentAttackCooldown_ -= delta;
    }

    // 随机生成怪物
    monsterSpawnTimer_ += delta;
    if (monsterSpawnTimer_ > 10.0f) // 每10秒检查一次生成
    {
        monsterSpawnTimer_ = 0;
        if (monsters_.size() < 15 && rand() % 100 < getMonsterSpawnChance() * 100)
        {
            spawnMonster();
        }
    }
}

void MineScene::updateCamera()
{
    if (!player_) return;

    auto camera = this->getDefaultCamera();
    if (!camera) return;

    Vec2 playerPos = player_->getPosition();
    Vec3 currentPos = camera->getPosition3D();
    Vec3 targetPos = Vec3(playerPos.x, playerPos.y, currentPos.z);

    // 平滑跟随
    float smoothFactor = 0.1f;
    Vec3 newPos = currentPos + (targetPos - currentPos) * smoothFactor;

    // 限制范围
    if (mineLayer_)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        Size mapSize = mineLayer_->getMapSize();

        float minX = visibleSize.width / 2;
        float maxX = mapSize.width - visibleSize.width / 2;
        float minY = visibleSize.height / 2;
        float maxY = mapSize.height - visibleSize.height / 2;

        if (mapSize.width < visibleSize.width) minX = maxX = mapSize.width / 2;
        if (mapSize.height < visibleSize.height) minY = maxY = mapSize.height / 2;

        newPos.x = std::max(minX, std::min(maxX, newPos.x));
        newPos.y = std::max(minY, std::min(maxY, newPos.y));
    }

    camera->setPosition3D(newPos);

    // 更新UI位置
    if (uiLayer_)
    {
        Vec2 uiPos = Vec2(
            newPos.x - Director::getInstance()->getVisibleSize().width / 2,
            newPos.y - Director::getInstance()->getVisibleSize().height / 2
        );
        uiLayer_->setPosition(uiPos);
    }
}

void MineScene::updateUI()
{
    // 更新工具显示
    if (inventory_ && itemLabel_)
    {
        ItemType type = inventory_->getSlot(selectedItemIndex_).type;
        std::string name = InventoryManager::getItemName(type);
        if (type == ItemType::ITEM_NONE) name = "Empty";

        // 显示选中状态 [1] Pickaxe
        int num = (selectedItemIndex_ + 1) % 10;
        if (num == 0) num = 10; // 显示习惯 1-0

        itemLabel_->setString(StringUtils::format("[%d] %s", selectedItemIndex_ == 9 ? 0 : selectedItemIndex_ + 1, name.c_str()));
    }

    // 更新血量显示 (假设 uiLayer 有 healthLabel_, 如果没有需要 initUI 添加)
    // 之前 initUI 只有 floorLabel_ 等。如果有 healthLabel_ 最好。
    // 检查 initUI 发现没有创建 healthLabel_ 的代码，这需要补上.
    // 我们在这里动态添加？或者依赖 initUI 的修改。
    // 为了稳妥，在这里检查并创建，或者复用 positionLabel_ 暂时显示?
    // 查看 initUI，有 floorLabel_, itemLabel_, actionLabel_
    // 我们在 initUI chunk 中已经添加了 healthLabel_ 的声明？
    // 看起来 MineScene.h 中有 healthLabel_，但 cpp initUI 没初始化。

    if (!healthLabel_ && uiLayer_)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        healthLabel_ = Label::createWithSystemFont("HP: 100/100", "Arial", 20);
        healthLabel_->setAnchorPoint(Vec2(0, 0.5));
        healthLabel_->setPosition(Vec2(origin.x + 20, origin.y + visibleSize.height - 50));
        healthLabel_->setColor(Color3B::RED);
        uiLayer_->addChild(healthLabel_, 1);
    }

    if (healthLabel_ && player_)
    {
        healthLabel_->setString(StringUtils::format("HP: %d/%d", player_->getHp(), 100)); // 假定已添加 getMaxHp()
    }
    refreshToolbarUI();
}

void MineScene::updateMonsters(float delta)
{
    // 移除死亡怪物
    for (auto it = monsters_.begin(); it != monsters_.end(); )
    {
        Monster* monster = *it;
        if (monster->isDead())
        {
            // 简单的移除逻辑，Monster类自己处理了FadeOut和RemoveSelf
            // 我们只需要从列表中移除引用
            // TODO: 掉落战利品
            int dropChance = 30 + currentFloor_ * 5;
            if (rand() % 100 < dropChance)
            {
                // 掉落
                // GameScene::spawnItem(ItemType::Coal, monster->getPosition(), 1); 
                // 由于无法直接调用GameScene方法，这里暂时省略掉落物捡起逻辑
                // 可以添加一个 ItemNode 类来在场景中显示掉落物
            }

            it = monsters_.erase(it);
        }
        else
        {
            // AI 更新已经在 Monster::update 中调用
            // 我们这里只需要检查碰撞伤害
            if (player_ && !player_->isInvulnerable())
            {
                float dist = player_->getPosition().distance(monster->getPosition());
                if (dist < 30.0f) // 碰撞范围
                {
                    // 玩家受伤
                    player_->takeDamage(monster->getAttackPower()); // 调用真实扣血

                    // 刷新UI
                    updateUI();
                    // 简单击退
                    Vec2 pushDir = player_->getPosition() - monster->getPosition();
                    pushDir.normalize();
                    player_->setPosition(player_->getPosition() + pushDir * 20.0f);

                    showActionMessage("Ouch!", Color3B::RED);
                }
            }
            ++it;
        }
    }
}


void MineScene::initToolbar()
{
    // 初始化工具栏物品 (使用 ID 0-7)
    toolbarItems_.clear();

    // 确保背包里有基础工具（测试用）
    if (inventory_)
    {
        // 检查是否有剑和镐
        bool hasSword = false;
        bool hasPickaxe = false;
        for (int i=0; i<8; ++i) {
            ItemType t = inventory_->getSlot(i).type;
            if (t == ItemType::ITEM_WoodenSword || t == ItemType::ITEM_IronSword ||
                t == ItemType::ITEM_GoldSword || t == ItemType::ITEM_DiamondSword) hasSword = true;
            if (t == ItemType::Pickaxe) hasPickaxe = true;
        }

        if (!hasSword) {
            inventory_->addItem(ItemType::ITEM_WoodenSword, 1);
            CCLOG("Starter Kit: Added Sword");
        }
        if (!hasPickaxe) {
            inventory_->addItem(ItemType::Pickaxe, 1);
            CCLOG("Starter Kit: Added Pickaxe");
        }
        
        for (int i = 0; i < 8; ++i)
        {
            auto slot = inventory_->getSlot(i);
            toolbarItems_.push_back(slot.type);
        }
    }
    else
    {
        // Fallback if no inventory
        for (int i = 0; i < 8; ++i) toolbarItems_.push_back(ItemType::ITEM_NONE);
    }

    // 恢复选中的物品
    if (inventory_)
    {
        selectedItemIndex_ = inventory_->getSelectedSlotIndex();
        if (selectedItemIndex_ < 0 || selectedItemIndex_ >= static_cast<int>(toolbarItems_.size()))
        {
            selectedItemIndex_ = 0;
        }
    }
    else
    {
        selectedItemIndex_ = 0;
    }

    // 如果有UI，选中默认
    this->selectItemByIndex(selectedItemIndex_);
}

void MineScene::initToolbarUI()
{
    if (!uiLayer_ || toolbarItems_.empty()) {
        return;
    }

    if (toolbarUI_) {
        toolbarUI_->removeFromParent();
        toolbarUI_ = nullptr;
        toolbarSlots_.clear();
        toolbarIcons_.clear();
        toolbarCounts_.clear();
        toolbarCountCache_.clear();
        toolbarSelectedCache_ = -1;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    int slotCount = static_cast<int>(toolbarItems_.size());
    float barWidth = slotCount * kToolbarSlotSize + (slotCount + 1) * kToolbarSlotPadding;
    float barHeight = kToolbarSlotSize + kToolbarSlotPadding * 2.0f;

    toolbarUI_ = LayerColor::create(kToolbarBarColor, barWidth, barHeight);
    toolbarUI_->setPosition(Vec2(
        origin.x + (visibleSize.width - barWidth) * 0.5f,
        origin.y + 8.0f));
    uiLayer_->addChild(toolbarUI_, 2);

    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(barWidth, barHeight),
        Color4F(0.5f, 0.45f, 0.4f, 1.0f));
    border->setLineWidth(2);
    toolbarUI_->addChild(border, 1);

    toolbarSlots_.reserve(slotCount);
    toolbarIcons_.reserve(slotCount);
    toolbarCounts_.reserve(slotCount);
    toolbarCountCache_.assign(slotCount, -1);

    for (int i = 0; i < slotCount; ++i)
    {
        auto slotBg = Sprite::create();
        slotBg->setAnchorPoint(Vec2(0, 0));
        slotBg->setTextureRect(Rect(0, 0, kToolbarSlotSize, kToolbarSlotSize));
        slotBg->setColor(kToolbarSlotColor);
        slotBg->setPosition(Vec2(
            kToolbarSlotPadding + i * (kToolbarSlotSize + kToolbarSlotPadding),
            kToolbarSlotPadding));

        auto slotBorder = DrawNode::create();
        slotBorder->drawRect(
            Vec2(0, 0),
            Vec2(kToolbarSlotSize, kToolbarSlotSize),
            Color4F(0.35f, 0.3f, 0.25f, 1.0f));
        slotBorder->setLineWidth(2);
        slotBg->addChild(slotBorder, 1);

        toolbarUI_->addChild(slotBg, 2);
        toolbarSlots_.push_back(slotBg);

        auto icon = createToolbarIcon(toolbarItems_[i]);
        if (icon) {
            icon->setPosition(Vec2(kToolbarSlotSize * 0.5f, kToolbarSlotSize * 0.5f));
            slotBg->addChild(icon, 0);
        }
        toolbarIcons_.push_back(icon);

        auto countLabel = Label::createWithSystemFont("", "Arial", 14);
        countLabel->setAnchorPoint(Vec2(1, 0));
        countLabel->setPosition(Vec2(kToolbarSlotSize - 4.0f, 4.0f));
        countLabel->setColor(Color3B::WHITE);
        slotBg->addChild(countLabel, 2);
        toolbarCounts_.push_back(countLabel);
    }

    refreshToolbarUI();
}

void MineScene::refreshToolbarUI()
{
    if (toolbarSlots_.empty()) {
        return;
    }

    if (inventory_)
    {
        int maxSlots = std::min(static_cast<int>(toolbarItems_.size()), inventory_->getSlotCount());
        for (int i = 0; i < maxSlots; ++i)
        {
            const auto& slot = inventory_->getSlot(i);
            ItemType newType = slot.isEmpty() ? ItemType::ITEM_NONE : slot.type;

            if (toolbarItems_[i] != newType)
            {
                toolbarItems_[i] = newType;

                if (i < static_cast<int>(toolbarIcons_.size()) && toolbarIcons_[i])
                {
                    toolbarIcons_[i]->removeFromParent();
                    toolbarIcons_[i] = nullptr;
                }

                auto icon = createToolbarIcon(newType);
                if (icon && i < static_cast<int>(toolbarSlots_.size()))
                {
                    icon->setPosition(Vec2(kToolbarSlotSize * 0.5f, kToolbarSlotSize * 0.5f));
                    toolbarSlots_[i]->addChild(icon, 0);
                }

                if (i < static_cast<int>(toolbarIcons_.size()))
                {
                    toolbarIcons_[i] = icon;
                }

                if (i < static_cast<int>(toolbarCountCache_.size()))
                {
                    toolbarCountCache_[i] = -1;
                }
            }
        }
    }

    if (toolbarSelectedCache_ != selectedItemIndex_) {
        for (int i = 0; i < static_cast<int>(toolbarSlots_.size()); ++i) {
            bool isSelected = (i == selectedItemIndex_);
            toolbarSlots_[i]->setColor(isSelected ? kToolbarSlotSelectedColor : kToolbarSlotColor);
        }
        toolbarSelectedCache_ = selectedItemIndex_;
    }

    if (toolbarCountCache_.size() != toolbarItems_.size()) {
        toolbarCountCache_.assign(toolbarItems_.size(), -1);
    }

    for (int i = 0; i < static_cast<int>(toolbarItems_.size()); ++i)
    {
        if (i >= static_cast<int>(toolbarCounts_.size())) {
            break;
        }

        int count = -1;
        if (inventory_ && i < inventory_->getSlotCount()) {
            const auto& slot = inventory_->getSlot(i);
            if (!slot.isEmpty() && InventoryManager::isStackable(slot.type)) {
                count = slot.count;
            }
        }

        if (toolbarCountCache_[i] != count) {
            toolbarCountCache_[i] = count;
            if (count > 1) {
                toolbarCounts_[i]->setString(StringUtils::format("%d", count));
            }
            else {
                toolbarCounts_[i]->setString("");
            }
        }
    }
}

void MineScene::selectItemByIndex(int idx)
{
    if (idx < 0 || idx >= 8) return;
    selectedItemIndex_ = idx;

    // 重新获取当前工具类型（因为背包可能变动）
    if (inventory_) {
        toolbarItems_[idx] = inventory_->getSlot(idx).type;
        // 同步选中状态到 InventoryManager
        inventory_->setSelectedSlotIndex(idx);
    }

    if (player_) {
        ItemType currentItem = ItemType::ITEM_NONE;
        if (inventory_ && idx >= 0 && idx < inventory_->getSlotCount()) {
            currentItem = inventory_->getSlot(idx).type;
        }
        player_->setCurrentTool(currentItem);
    }

    updateUI();
}

void MineScene::toggleInventory()
{
    if (inventoryUI_)
    {
        inventoryUI_->close(); // 应该会自动调用 onInventoryClosed 通过回调
        // 如果没有设置回调，手动置空，但保险起见在 onInventoryClosed 处理
        return;
    }

    // 创建背包UI
    inventoryUI_ = InventoryUI::create(inventory_);
    if (!inventoryUI_) return;

    // 设置回调
    inventoryUI_->setCloseCallback([this]() {
        onInventoryClosed();
        });

    // 添加到 uiLayer_ 或者直接 Scene
    // uiLayer_ 的 ZOrder 是 1000，背包需要在更上面
    // 但是 uiLayer_ 跟随摄像机移动，InventoryUI 通常也是跟随摄像机的或者固定在屏幕空间
    // 这里我们把 InventoryUI 添加到 TOP
    if (uiLayer_) {
        uiLayer_->addChild(inventoryUI_, 100); // 相对 uiLayer 的 Top
        inventoryUI_->setPosition(Vec2::ZERO); // 覆盖整个屏幕
    }
    else {
        this->addChild(inventoryUI_, 2000);
    }

    inventoryUI_->show();
}

void MineScene::onInventoryClosed()
{
    inventoryUI_ = nullptr;
    // 背包关闭后，刷新工具栏数据
    initToolbar();
    refreshToolbarUI();
}

void MineScene::executeMining(const Vec2& tileCoord)
{
    MiningManager::MiningResult result = miningManager_->mineTile(tileCoord);
    if (result.success)
    {
        player_->playSwingAnimation();
        player_->consumeEnergy(4.0f);
        if (!result.message.empty()) {
            showActionMessage(result.message, Color3B::GREEN);
        }
    }
}

void MineScene::handleMiningAction()
{
    // 1. 基础指针与状态检查
    if (!player_ || !miningManager_ || !mineLayer_ || !inventory_) return;

    // 2. 体力检查
    float energyPercent = player_->getCurrentEnergy() / player_->getMaxEnergy();
    if (energyPercent <= 0.2f)
    {
        showActionMessage("Exhausted!", Color3B::RED);
        return;
    }

    // 3. 工具检查 (必须是 Pickaxe)
    ItemType currentTool = inventory_->getSlot(selectedItemIndex_).type;
    if (currentTool != ItemType::Pickaxe)
    {
        showActionMessage("Need a Pickaxe!", Color3B::RED);
        handleAttackAction(); // 如果不是镐子，尝试执行攻击
        return;
    }

    // 4. 【核心修改】九宫格检测逻辑
    // 获取玩家当前所在的瓦片坐标 (Tile Coordinate)
    Vec2 playerPos = player_->getPosition();
    Vec2 playerTileCoord = mineLayer_->positionToTileCoord(playerPos);

    bool mined = false;

    // 疲劳延迟逻辑 (保留原逻辑)
    float delay = 0.0f;
    if (energyPercent <= 0.5f) delay = 0.2f;

    // 定义九宫格的偏移量 (包含中心点(0,0)以及周围8个方向)
    std::vector<Vec2> offsets = {
        Vec2(0, 0),   // 脚下
        Vec2(1, 0),   // 右
        Vec2(-1, 0),  // 左
        Vec2(0, 1),   // 下 (Tiled坐标系Y轴方向取决于地图配置，但九宫格覆盖所有相邻)
        Vec2(0, -1),  // 上
        Vec2(1, 1),   // 右下
        Vec2(1, -1),  // 右上
        Vec2(-1, 1),  // 左下
        Vec2(-1, -1)  // 左上
    };

    for (const auto& offset : offsets)
    {
        // 计算目标瓦片坐标
        Vec2 targetTile = playerTileCoord + offset;

        // 检查该位置是否有矿石
        if (mineLayer_->isMineralAt(targetTile))
        {
            mined = true;

            // 播放挥舞动画
            player_->playSwingAnimation();

            // 执行挖掘逻辑 (带延迟处理)
            if (delay > 0) {
                auto seq = Sequence::create(
                    DelayTime::create(delay),
                    CallFunc::create([this, targetTile]() {
                        executeMining(targetTile);
                        }),
                    nullptr
                );
                this->runAction(seq);
            }
            else {
                executeMining(targetTile);
            }

            // 找到一个矿石就退出，避免一次操作挖掉周围所有矿石 (如果想群挖，可以删除 break)
            break;
        }
    }

    // 5. 如果周围九格都没有矿石，执行空挥动画
    if (!mined)
    {
        player_->playSwingAnimation();
    }
}


void MineScene::handleAttackAction()
{
    if (currentAttackCooldown_ > 0) return;

    // 播放攻击动画
    player_->playSwingAnimation();
    currentAttackCooldown_ = attackCooldown_;

    Vec2 playerPos = player_->getPosition();
    float attackRange = 50.0f; // 默认范围
    int attackDamage = 1;      // 默认空手伤害

    // 获取当前装备的武器
    if (inventory_)
    {
        ItemType item = inventory_->getSlot(selectedItemIndex_).type;
        // 检查是否是武器 (简单的列表检查，或者在 ItemType 中有分类)
        // 假设 Sword 相关的都是
        if (item == ItemType::ITEM_WoodenSword || item == ItemType::ITEM_IronSword ||
            item == ItemType::ITEM_GoldSword || item == ItemType::ITEM_DiamondSword)
        {
            // 使用 Weapon 类获取伤害
            attackDamage = Weapon::getWeaponAttackPower(item);
            attackRange = Weapon::getWeaponAttackRange(item);
        }
        else if (item == ItemType::Pickaxe || item == ItemType::Axe || item == ItemType::Scythe)
        {
            // 工具也能打人，但伤害低
            attackDamage = 3;
            attackRange = 40.0f;
        }
        else
        {
            // 没拿武器只能空手
            // 如果玩家意图是攻击（比如按了J），但没拿武器，提示一下？
            // 或者允许空手搏斗
            // 这里我们允许空手，伤害为1
        }
    }

    bool hit = false;
    for (auto monster : monsters_)
    {
        if (monster->isDead()) continue;

        float dist = playerPos.distance(monster->getPosition());
        if (dist <= attackRange)
        {
            monster->takeDamage(attackDamage);
            hit = true;

            // 击退
            Vec2 knockback = monster->getPosition() - playerPos;
            knockback.normalize();
            monster->setPosition(monster->getPosition() + knockback * 30.0f);
        }
    }

    if (hit)
    {
        showActionMessage("Hit!", Color3B::ORANGE);
    }
}

void MineScene::handleChestInteraction()
{
    if (!player_) return;

    Vec2 playerPos = player_->getPosition();

    for (auto chest : chests_)
    {
        if (chest->isOpened()) continue;

        float dist = playerPos.distance(chest->getPosition());
        if (dist < 40.0f)
        {
            auto result = chest->open();
            if (result.item != ItemType::ITEM_NONE)
            {
                if (inventory_->addItem(result.item, result.count))
                {
                    showActionMessage(result.message, Color3B::YELLOW);

                    // 记录开启状态（按周记录持久化）
                    int dayCount = 1;
                    if (TimeManager::getInstance()) dayCount = TimeManager::getInstance()->getDay();
                    int currentWeek = (dayCount - 1) / 7 + 1;
                    openedChestsPerWeek_[currentFloor_] = currentWeek;

                }
            }
            return;
        }
    }
}

void MineScene::showActionMessage(const std::string& text, const Color3B& color)
{
    if (!actionLabel_) return;

    actionLabel_->setString(text);
    actionLabel_->setColor(color);
    actionLabel_->setOpacity(255);
    actionLabel_->stopAllActions();

    auto seq = Sequence::create(
        DelayTime::create(0.5f),
        FadeOut::create(1.0f),
        nullptr
    );
    actionLabel_->runAction(seq);
}

void MineScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_ESCAPE:
        backToFarm();
        break;

    case EventKeyboard::KeyCode::KEY_J:
        CCLOG(">>> Key 'J' pressed in MineScene::onKeyPressed");
        handleMiningAction();
        break;

    case EventKeyboard::KeyCode::KEY_SPACE:
        handleChestInteraction();
        break;

    case EventKeyboard::KeyCode::KEY_ENTER:
    case EventKeyboard::KeyCode::KEY_KP_ENTER:
        if (isPlayerOnStairs())
        {
            goToNextFloor();
        }
        else
        {
            showActionMessage("Not on stairs!", Color3B::GRAY);
        }
        break;

    case EventKeyboard::KeyCode::KEY_M:
        // [Optimization 4] 电梯交互限制：必须在电梯附近
        if (elevatorSprite_)
        {
            float dist = player_->getPosition().distance(elevatorSprite_->getPosition());
            if (dist < 60.0f) // 判定范围
            {
                showElevatorUI();
            }
            else
            {
                showActionMessage("Too far from elevator!", Color3B::GRAY);
            }
        }
        else
        {
            // 如果没电梯，暂时允许直接回城 (Fallback)
            backToFarm();
        }
        break;

        /* [Removed Shortcuts]
        case EventKeyboard::KeyCode::KEY_Q:
            goToPreviousFloor();
            break;

        case EventKeyboard::KeyCode::KEY_E:
            goToNextFloor();
            break;
        */

    case EventKeyboard::KeyCode::KEY_TAB:
    {
        CCLOG("Cheat: Skipping Day from Mine...");
        auto tm = TimeManager::getInstance();
        if (tm) {
             tm->skipToNextMorning();
             Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HouseScene::createScene(true)));
        }
        break;
    }
    case EventKeyboard::KeyCode::KEY_B:
        toggleInventory();
        break;

        // 数字键选择物品
    case EventKeyboard::KeyCode::KEY_1: selectItemByIndex(0); break;
    case EventKeyboard::KeyCode::KEY_2: selectItemByIndex(1); break;
    case EventKeyboard::KeyCode::KEY_3: selectItemByIndex(2); break;
    case EventKeyboard::KeyCode::KEY_4: selectItemByIndex(3); break;
    case EventKeyboard::KeyCode::KEY_5: selectItemByIndex(4); break;
    case EventKeyboard::KeyCode::KEY_6: selectItemByIndex(5); break;
    case EventKeyboard::KeyCode::KEY_7: selectItemByIndex(6); break;
    case EventKeyboard::KeyCode::KEY_8: selectItemByIndex(7); break;

    default:
        break;
    }
}

void MineScene::backToFarm()
{
    CCLOG("Returning to farm, loading from save...");
    // 从存档加载主世界，保持之前的游戏状态
    auto gameScene = GameScene::createScene(true);  // true = 从存档加载
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, gameScene));
}

void MineScene::goToPreviousFloor()
{
    int prevFloor = currentFloor_ - 1;

    // 限制在 1-5 层之间循环
    if (prevFloor < 1)
    {
        prevFloor = 5;  // 从第 1 层按 Q 回到第 5 层
    }

    CCLOG("Switching to previous floor: %d -> %d", currentFloor_, prevFloor);
    auto prevScene = MineScene::createScene(inventory_, prevFloor);
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, prevScene));

}

void MineScene::goToNextFloor()
{
    int nextFloor = currentFloor_ + 1;

    // 限制在 1-5 层之间循环
    if (nextFloor > 5)
    {
        nextFloor = 1;  // 从第 5 层按 E 回到第 1 层
    }

    CCLOG("Switching to next floor: %d -> %d", currentFloor_, nextFloor);
    auto nextScene = MineScene::createScene(inventory_, nextFloor);
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, nextScene));

}

bool MineScene::isPlayerOnStairs() const
{
    if (!player_ || !mineLayer_) return false;

    Vec2 playerPos = player_->getPosition();
    Vec2 tileCoord = mineLayer_->positionToTileCoord(playerPos);

    return mineLayer_->isStairsAt(tileCoord);
}

void MineScene::spawnMonster()
{
    if (!mineLayer_ || !player_) return;

    Vec2 pos = getRandomWalkablePosition();

    // 确保不在玩家太近的地方生成
    if (pos.distance(player_->getPosition()) < 200.0f) return;

    Monster* monster = nullptr;

    // 根据楼层决定生成的怪物类型
    int roll = rand() % 100;

    // 僵尸生成概率随楼层增加
    int zombieChance = (currentFloor_ - 1) * 20;
    if (zombieChance > 80) zombieChance = 80;

    if (roll < zombieChance)
    {
        monster = Zombie::create(currentFloor_);
    }
    else
    {
        monster = Slime::create(currentFloor_);
    }

    if (monster)
    {
        monster->setPosition(pos);
        monster->setTargetPlayer(player_);
        monster->setMapLayer(mineLayer_);

        this->addChild(monster, 10);
        monsters_.push_back(monster);

        CCLOG("Spawned %s at (%.1f, %.1f)", monster->getMonsterName().c_str(), pos.x, pos.y);
    }
}

float MineScene::getMonsterSpawnChance() const
{
    // 楼层越深概率越大
    return 0.1f + currentFloor_ * 0.05f;
}

float MineScene::getChestSpawnChance() const
{
    return 0.2f + currentFloor_ * 0.05f;
}

Vec2 MineScene::getRandomWalkablePosition() const
{
    if (!mineLayer_) return Vec2::ZERO;

    Size mapSize = mineLayer_->getMapSize();
    int maxAttempts = 50;

    for (int i = 0; i < maxAttempts; ++i)
    {
        float x = (rand() % (int)mapSize.width);
        float y = (rand() % (int)mapSize.height);
        Vec2 pos(x, y);

        if (mineLayer_->isWalkable(pos))
        {
            return pos;
        }
    }

    return Vec2(mapSize.width / 2, mapSize.height / 2);
}





void MineScene::initWishingWell()
{
    wishingWell_ = nullptr;
    if (!mineLayer_) return;

    // 限制只在 5 的倍数层出现
    if (currentFloor_ % 5 != 0) return;

    // 增加概率 (例如 50%)
    if (rand() % 100 < 50) return;

    // 在地图上找一个位置放置许愿池
    Vec2 pos = getRandomWalkablePosition();

    // 创建许愿池视觉 (蓝色圆形)
    auto wellNode = DrawNode::create();
    wellNode->drawDot(Vec2::ZERO, 20, Color4F(0.2f, 0.4f, 1.0f, 0.8f)); // 水池
    wellNode->drawCircle(Vec2::ZERO, 22, 0, 30, false, Color4F::GRAY); // 边框

    wishingWell_ = Node::create();
    wishingWell_->setPosition(pos);
    wishingWell_->addChild(wellNode);
    this->addChild(wishingWell_, 5); // 与宝箱同层

    // 提示文字
    auto label = Label::createWithSystemFont("Wishing Well\n(Press K)", "Arial", 12);
    label->setPosition(Vec2(0, 30));
    label->setAlignment(TextHAlignment::CENTER);
    wishingWell_->addChild(label);

    CCLOG("Wishing Well initialized at (%.1f, %.1f)", pos.x, pos.y);
}

void MineScene::handleWishAction()
{
    if (!player_ || !wishingWell_ || !inventory_) return;

    // 检查距离
    float dist = player_->getPosition().distance(wishingWell_->getPosition());
    if (dist > 60.0f)
    {
        showActionMessage("Too far from Wishing Well!", Color3B::GRAY);
        return;
    }

    // 检查当前手持物品
    ItemType currentItem = inventory_->getSlot(selectedItemIndex_).type;
    if (currentItem == ItemType::ITEM_NONE)
    {
        showActionMessage("Hold an item to wish!", Color3B::YELLOW);
        return;
    }

    // 扣除物品
    if (inventory_->removeItem(currentItem, 1))
    {
        // 播放效果（简单震动或颜色变化）
        wishingWell_->runAction(Sequence::create(
            ScaleTo::create(0.1f, 1.2f),
            ScaleTo::create(0.1f, 1.0f),
            nullptr
        ));

        // 随机奖励
        int randVal = rand() % 100;
        if (randVal < 30) // 30% 啥也没有
        {
            showActionMessage("The well is silent...", Color3B::GRAY);
        }
        else if (randVal < 70) // 40% 金币 / 回血
        {
            if (rand() % 2 == 0)
            {
                int gold = 50 + rand() % 151; // 50-200
                inventory_->addMoney(gold);
                showActionMessage(StringUtils::format("Well grants %d Gold!", gold), Color3B::YELLOW);
            }
            else
            {
                int heal = 20 + rand() % 31; // 20-50
                player_->heal(heal);
                showActionMessage("You feel refreshed!", Color3B::GREEN);
            }
        }
        else // 30% 好东西
        {
            // 随机给个矿石或更稀有的
            ItemType rewards[] = { ItemType::GoldOre, ItemType::ITEM_DiamondSword, ItemType::ITEM_GoldSword };
            ItemType reward = rewards[rand() % 3];

            // 如果是武器且已有，折算成钱
            if ((reward == ItemType::ITEM_DiamondSword || reward == ItemType::ITEM_GoldSword) && inventory_->hasItem(reward, 1))
            {
                int gold = 500;
                inventory_->addMoney(gold);
                showActionMessage("Great fortune! (500 Gold)", Color3B::ORANGE);
            }
            else
            {
                if (inventory_->addItem(reward, 1))
                {
                    showActionMessage(StringUtils::format("Received %s!", InventoryManager::getItemName(reward).c_str()), Color3B::MAGENTA);
                }
                else
                {
                    //背包满了
                    int gold = Weapon::getWeaponPrice(reward);
                    inventory_->addMoney(gold);
                    showActionMessage("Bag full, took Gold instead.", Color3B::ORANGE);
                }
            }
        }

        // 刷新UI
        updateUI();
    }
}

// Elevator Helpers
void MineScene::showElevatorUI()
{
    if (elevatorUI_) {
        elevatorUI_->close();
        elevatorUI_ = nullptr;
    }

    elevatorUI_ = ElevatorUI::create();
    if (elevatorUI_)
    {
        elevatorUI_->setFloorSelectCallback(CC_CALLBACK_1(MineScene::onElevatorFloorSelected, this));
        elevatorUI_->setCloseCallback(CC_CALLBACK_0(MineScene::onElevatorClosed, this));

        // 添加到 uiLayer_ 以保证其在屏幕正中心显示
        if (uiLayer_) {
            uiLayer_->addChild(elevatorUI_, 2000);
            elevatorUI_->setPosition(Vec2::ZERO);
        }
        else {
            this->addChild(elevatorUI_, 2000);
        }

        elevatorUI_->show();
    }
}

void MineScene::onElevatorClosed()
{
    elevatorUI_ = nullptr;
}

void MineScene::onElevatorFloorSelected(int floor)
{
    if (floor == 0)
    {
        backToFarm();
    }
    else if (floor >= 1 && floor <= 5) // Limits could be dynamic
    {
        CCLOG("Elevator to floor %d", floor);
        auto nextScene = MineScene::createScene(inventory_, floor);
        Director::getInstance()->replaceScene(TransitionFade::create(0.5f, nextScene));

    }
    else
    {
        showActionMessage("Floor unavailable!", Color3B::RED);
    }
}
