#include "MineScene.h"
#include "MineLayer.h"
#include "Player.h"
#include "InventoryManager.h"
#include "InventoryUI.h"
#include "MiningManager.h"
#include "Monster.h"
#include "TreasureChest.h"
#include "Weapon.h"
#include "SaveManager.h"
#include "SkillManager.h"
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

    bool syncSaveInventoryAndSkills(InventoryManager* inventory)
    {
        if (!inventory)
            return false;

        auto saveMgr = SaveManager::getInstance();
        if (!saveMgr || !saveMgr->hasSaveFile())
            return false;

        SaveManager::SaveData data;
        if (!saveMgr->loadGame(data))
            return false;

        data.inventory.slots.clear();
        int slotCount = inventory->getSlotCount();
        data.inventory.slots.reserve(slotCount);
        for (int i = 0; i < slotCount; ++i)
        {
            const auto& slot = inventory->getSlot(i);
            SaveManager::SaveData::InventoryData::ItemSlotData slotData;
            if (!slot.isEmpty())
            {
                slotData.type = static_cast<int>(slot.type);
                slotData.count = slot.count;
                slotData.durability = slot.durability;
                slotData.maxDurability = slot.maxDurability;
            }
            else
            {
                slotData.type = static_cast<int>(ItemType::ITEM_NONE);
                slotData.count = 0;
                slotData.durability = -1;
                slotData.maxDurability = -1;
            }
            data.inventory.slots.push_back(slotData);
        }
        data.inventory.money = inventory->getMoney();

        if (auto tm = TimeManager::getInstance())
        {
            data.dayCount = tm->getDay();
        }

        if (auto skillMgr = SkillManager::getInstance())
        {
            data.skills.clear();
            for (int i = 0; i < static_cast<int>(SkillManager::SkillType::Count); ++i)
            {
                auto type = static_cast<SkillManager::SkillType>(i);
                const auto& sd = skillMgr->getSkillData(type);
                SaveManager::SaveData::SkillData skillData;
                skillData.type = i;
                skillData.level = sd.level;
                skillData.actionCount = sd.actionCount;
                data.skills.push_back(skillData);
            }
        }

        return saveMgr->saveGame(data);
    }
}

// 
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

    // 
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

    // 
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
    initToolbarUI(); // 

    // 
    this->scheduleUpdate();

    CCLOG("Mine Scene initialized successfully!");
    return true;
}

void MineScene::initMap()
{
    CCLOG("Initializing mine map...");

    //  Mines 
    std::string mapFile = StringUtils::format("map/Mines/%d.tmx", currentFloor_);

    mineLayer_ = MineLayer::create(mapFile);
    if (mineLayer_)
    {
        this->addChild(mineLayer_, 0);
        CCLOG("Mine layer added to scene (Floor %d)", currentFloor_);

        //  Z-order
        // Front -> Back -> Buildings -> (Z=10) -> mine1
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

            // 
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

            //  Front  (/)
            auto frontLayer = tmxMap->getLayer("Front");
            if (frontLayer)
            {
                CCLOG(" Front layer found!");
                // Front  (Roof/TreeTop)
                //  20 (Player is 10)
                frontLayer->setLocalZOrder(20);
                frontLayer->setVisible(true);
                frontLayer->setOpacity(255);
                CCLOG(" Front layer set to Z-order 20 (Above Player)");
            }
            else
            {
                CCLOG(" WARNING: Front layer not found in TMX map");
            }

            //  Back  Front 
            //  Back 
            auto backLayer = tmxMap->getLayer("Back");
            if (backLayer)
            {
                CCLOG(" Back layer found - keeping in tmxMap");
                CCLOG("  - Original opacity: %d", backLayer->getOpacity());
                CCLOG("  - Layer size: %.0f x %.0f tiles", backLayer->getLayerSize().width, backLayer->getLayerSize().height);

                // 
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
                            if (nonZeroCount <= 3)  // 3
                            {
                                CCLOG("    Sample tile at (%d,%d): GID=%d", x, y, gid);
                            }
                        }
                    }
                }
                CCLOG("  - Tile sampling: %d/%d tiles are non-zero in top-left area", nonZeroCount, sampleCount);

                backLayer->setLocalZOrder(-100);  //  Front 
                backLayer->setVisible(true);
                backLayer->setOpacity(255);  // 

                CCLOG("  - Back layer visible: %s, opacity: %d",
                    backLayer->isVisible() ? "YES" : "NO",
                    backLayer->getOpacity());
            }

            //  Buildings / Back 
            auto buildingsLayer = tmxMap->getLayer("Buildings");
            if (buildingsLayer)
            {
                CCLOG(" Buildings layer found!");
                CCLOG("  - Visible: %s", buildingsLayer->isVisible() ? "YES" : "NO");
                CCLOG("  - Original opacity: %d", buildingsLayer->getOpacity());
                CCLOG("  - Layer Size: (%.0f, %.0f)", buildingsLayer->getLayerSize().width, buildingsLayer->getLayerSize().height);

                buildingsLayer->setLocalZOrder(-50);  //  Back 
                buildingsLayer->setVisible(true);  // 
                buildingsLayer->setOpacity(255);  // 
                CCLOG(" Buildings layer set to Z-order -50, forced visible, opacity: 255");
            }
            else
            {
                CCLOG(" WARNING: Buildings layer not found in TMX map");
            }

            //  mine1 
            auto mine1Layer = tmxMap->getLayer("mine1");
            if (mine1Layer)
            {
                CCLOG(" mine1 layer found!");
                CCLOG("  - Position: (%.2f, %.2f)", mine1Layer->getPosition().x, mine1Layer->getPosition().y);
                CCLOG("  - Visible: %s", mine1Layer->isVisible() ? "YES" : "NO");
                CCLOG("  - Original opacity: %d", mine1Layer->getOpacity());
                CCLOG("  - Layer Size: (%.0f, %.0f)", mine1Layer->getLayerSize().width, mine1Layer->getLayerSize().height);

                // mine1  tmxMap  Buildings 
                mine1Layer->setLocalZOrder(-25);  //  Buildings(-50) (10)
                mine1Layer->setVisible(true);
                mine1Layer->setOpacity(255);  // 
                CCLOG(" mine1 layer set to Z-order -25 (above Buildings, below Player), opacity: 255");
            }
            else
            {
                CCLOG(" WARNING: mine1 layer not found in TMX map");
            }
        }
        else
        {
            CCLOG(" WARNING: TMX map is null");
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

    // 
    player_ = Player::create();
    if (player_)
    {
        CCLOG("Initializing player in mine...");

        if (mineLayer_)
        {
            // 
            Size mapSize = mineLayer_->getMapSize();
            CCLOG("Map size: (%.2f, %.2f) pixels", mapSize.width, mapSize.height);

            // 
            Vec2 mapCenter = Vec2(mapSize.width / 2, mapSize.height / 2);
            CCLOG("Map center: (%.2f, %.2f)", mapCenter.x, mapCenter.y);

            // 
            Vec2 startPos = mapCenter;
            bool foundWalkable = false;

            // 
            if (mineLayer_->isWalkable(mapCenter))
            {
                foundWalkable = true;
                startPos = mapCenter;
                CCLOG(" Center position is walkable");
            }
            else
            {
                CCLOG("Center position not walkable, searching nearby...");

                // 
                for (int radius = 16; radius < 320 && !foundWalkable; radius += 16)
                {
                    //  8 
                    for (int angle = 0; angle < 360 && !foundWalkable; angle += 45)
                    {
                        float rad = angle * M_PI / 180.0f;
                        Vec2 testPos = mapCenter + Vec2(cos(rad) * radius, sin(rad) * radius);

                        // 
                        if (testPos.x >= 0 && testPos.x < mapSize.width &&
                            testPos.y >= 0 && testPos.y < mapSize.height)
                        {
                            if (mineLayer_->isWalkable(testPos))
                            {
                                startPos = testPos;
                                foundWalkable = true;
                                CCLOG(" Found walkable position at (%.2f, %.2f), radius: %d",
                                    testPos.x, testPos.y, radius);
                            }
                        }
                    }
                }
            }

            if (!foundWalkable)
            {
                CCLOG(" WARNING: Could not find walkable position, trying random...");
                startPos = getRandomWalkablePosition();
            }

            player_->setPosition(startPos);
            CCLOG(" Player positioned at (%.2f, %.2f)", startPos.x, startPos.y);
        }
        else
        {
            auto visibleSize = Director::getInstance()->getVisibleSize();
            player_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
            CCLOG(" WARNING: No map layer, using screen center");
        }

        player_->enableKeyboardControl();
        if (mineLayer_)
        {
            player_->setMapLayer(mineLayer_);
        }

        this->addChild(player_, 10);
        CCLOG(" Player added to scene");
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

    // 
    auto topBar = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, 40);
    topBar->setAnchorPoint(Vec2(0, 1));
    topBar->setPosition(Vec2(origin.x, origin.y + visibleSize.height));
    uiLayer_->addChild(topBar, 0);

    // 
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


    // 
    itemLabel_ = Label::createWithSystemFont("Tool: None", "Arial", 18);
    itemLabel_->setAnchorPoint(Vec2(1, 0.5));
    itemLabel_->setPosition(Vec2(origin.x + visibleSize.width - 20, origin.y + visibleSize.height - 20));
    itemLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(itemLabel_, 1);

    // 
    auto tipLabel = Label::createWithSystemFont("(Keys 1-8 to switch)", "Arial", 12);
    tipLabel->setAnchorPoint(Vec2(1, 0.5));
    tipLabel->setPosition(Vec2(origin.x + visibleSize.width - 20, origin.y + visibleSize.height - 40));
    tipLabel->setColor(Color3B::GRAY);
    uiLayer_->addChild(tipLabel, 1);

    // 
    actionLabel_ = Label::createWithSystemFont("", "Arial", 24);
    actionLabel_->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 100));
    actionLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(actionLabel_, 1);

    // 
    auto helpLabel = Label::createWithSystemFont(
        "WASD: Move | J: Attack/Mine | M: Elevator | ENTER: Stairs", "Arial", 14);
    helpLabel->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 20));
    helpLabel->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(helpLabel, 1);

    // 
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

    // 
    int initialCount = 1 + (rand() % 2); // 1 or 2
    if (currentFloor_ > 5) initialCount = 2; // 2

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

    // Create Mine Shaft Elevator () - 
    elevatorSprite_ = Sprite::create("myhouse/elevator.png");
    if (!elevatorSprite_) {
        // Create a compact and refined mine shaft entrance
        elevatorSprite_ = Sprite::create();
        auto draw = DrawNode::create();

        // 48x48 (70x70)
        const float size = 24.0f;  // 24px48px
        const float frameWidth = 4.0f;  // 
        const float innerSize = size - frameWidth;

        // 
        draw->drawSolidRect(Vec2(-size, -size), Vec2(size, size),
                           Color4F(0.25f, 0.15f, 0.08f, 1.0f));

        // 
        draw->drawSolidRect(Vec2(-innerSize, -innerSize), Vec2(innerSize, innerSize),
                           Color4F(0.05f, 0.05f, 0.05f, 1.0f));

        // 
        draw->drawSolidRect(Vec2(-size, -size), Vec2(-innerSize, size),
                           Color4F(0.45f, 0.30f, 0.18f, 1.0f));

        // 
        draw->drawSolidRect(Vec2(innerSize, -size), Vec2(size, size),
                           Color4F(0.35f, 0.22f, 0.13f, 1.0f));

        // 
        draw->drawSolidRect(Vec2(-size, innerSize), Vec2(size, size),
                           Color4F(0.40f, 0.26f, 0.15f, 1.0f));

        // 
        draw->drawSolidRect(Vec2(-size, -size), Vec2(size, -innerSize),
                           Color4F(0.30f, 0.20f, 0.12f, 1.0f));

        // 4
        for (int i = 0; i < 4; i++) {
            float y = -14 + i * 8;
            draw->drawSolidRect(Vec2(-14, y), Vec2(14, y + 1.5f),
                               Color4F(0.65f, 0.55f, 0.35f, 0.8f));
        }

        // 
        draw->drawSolidRect(Vec2(-14, -14), Vec2(-12, 14),
                           Color4F(0.55f, 0.45f, 0.30f, 0.7f));
        draw->drawSolidRect(Vec2(12, -14), Vec2(14, 14),
                           Color4F(0.55f, 0.45f, 0.30f, 0.7f));

        // "M"MINE
        auto label = Label::createWithSystemFont("M", "Arial", 10, Size::ZERO,
                                                 TextHAlignment::CENTER, TextVAlignment::CENTER);
        label->setPosition(Vec2(0, size + 8));
        label->setColor(Color3B(220, 180, 100));
        label->enableOutline(Color4B(40, 30, 20, 255), 1);
        elevatorSprite_->addChild(label, 10);

        // 
        auto glow = DrawNode::create();
        Vec2 glowPoints[] = {
            Vec2(-innerSize, -innerSize),
            Vec2(innerSize, -innerSize),
            Vec2(innerSize, innerSize),
            Vec2(-innerSize, innerSize)
        };
        glow->drawSolidPoly(glowPoints, 4, Color4F(0.3f, 0.25f, 0.15f, 0.3f));
        draw->addChild(glow, -1);

        elevatorSprite_->addChild(draw);
        elevatorSprite_->setContentSize(Size(48, 48));
    }

    elevatorSprite_->setPosition(centerPos);
    this->addChild(elevatorSprite_, 5);
}

void MineScene::initChests()
{
    chests_.clear();

    // 
    //  40% 
    int chestCount = (rand() % 100 < 40) ? 1 : 0;

    if (chestCount > 0)
    {
        // 1. 
        int dayCount = 1;
        if (TimeManager::getInstance()) dayCount = TimeManager::getInstance()->getDay();
        int currentWeek = (dayCount - 1) / 7 + 1;


        if (openedChestsPerWeek_.count(currentFloor_) > 0 &&
            openedChestsPerWeek_[currentFloor_] == currentWeek)
        {
            // 
            return;
        }

        // 2. 
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

    // 
    auto energyBar = this->getChildByName("EnergyBar");
    auto camera = this->getDefaultCamera();
    if (energyBar && camera)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        energyBar->setPosition(Vec2(camera->getPositionX() + visibleSize.width / 2 - 50,
            camera->getPositionY() - visibleSize.height / 2 + 110));
    }
    updateMonsters(delta);

    // 1. 
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

        // 2. 
        if (tm->isMidnight())
        {
            CCLOG("It's midnight! Passing out...");
            if (inventory_) inventory_->removeMoney(200);
            showActionMessage("Passed out...", Color3B::RED);
            Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HouseScene::createScene(true)));
            return;
        }
    }


    // 3.  ()
    if (player_ && player_->getHp() <= 0)
    {
        CCLOG("Player died in mine!");
        if (inventory_) inventory_->removeMoney(200);
        showActionMessage("You died...", Color3B::RED);
        Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HouseScene::createScene(true)));
        return;
    }

    // 
    if (currentAttackCooldown_ > 0)
    {
        currentAttackCooldown_ -= delta;
    }

    // 
    monsterSpawnTimer_ += delta;
    if (monsterSpawnTimer_ > 10.0f) // 10
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

    // 
    float smoothFactor = 0.1f;
    Vec3 newPos = currentPos + (targetPos - currentPos) * smoothFactor;

    // 
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

    // UI
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
    // 
    if (inventory_ && itemLabel_)
    {
        ItemType type = inventory_->getSlot(selectedItemIndex_).type;
        std::string name = InventoryManager::getItemName(type);
        if (type == ItemType::ITEM_NONE) name = "Empty";

        //  [1] Pickaxe
        int num = (selectedItemIndex_ + 1) % 10;
        if (num == 0) num = 10; //  1-0

        itemLabel_->setString(StringUtils::format("[%d] %s", selectedItemIndex_ == 9 ? 0 : selectedItemIndex_ + 1, name.c_str()));
    }

    //  ( uiLayer  healthLabel_,  initUI )
    //  initUI  floorLabel_  healthLabel_ 
    //  initUI  healthLabel_ .
    //  initUI 
    //  positionLabel_ ?
    //  initUI floorLabel_, itemLabel_, actionLabel_
    //  initUI chunk  healthLabel_ 
    //  MineScene.h  healthLabel_ cpp initUI 

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
        healthLabel_->setString(StringUtils::format("HP: %d/%d", player_->getHp(), 100)); //  getMaxHp()
    }
    refreshToolbarUI();
}

void MineScene::updateMonsters(float delta)
{
    // 
    for (auto it = monsters_.begin(); it != monsters_.end(); )
    {
        Monster* monster = *it;
        if (monster->isDead())
        {
            // MonsterFadeOutRemoveSelf
            // 
            // TODO: 
            int dropChance = 30 + currentFloor_ * 5;
            if (rand() % 100 < dropChance)
            {
                // 
                // GameScene::spawnItem(ItemType::Coal, monster->getPosition(), 1); 
                // GameScene
                //  ItemNode 
            }

            it = monsters_.erase(it);
        }
        else
        {
            // AI  Monster::update 
            // 
            if (player_ && !player_->isInvulnerable())
            {
                float dist = player_->getPosition().distance(monster->getPosition());
                if (dist < 30.0f) // 
                {
                    // 
                    player_->takeDamage(monster->getAttackPower()); // 

                    // UI
                    updateUI();
                    // 
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
    //  ( ID 0-7)
    toolbarItems_.clear();

    // 
    if (inventory_)
    {
        // 
        bool hasSword = false;
        bool hasPickaxe = false;
        for (int i = 0; i < inventory_->getSlotCount(); ++i) {
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

    // 
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

    // UI
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

    // 
    if (inventory_) {
        toolbarItems_[idx] = inventory_->getSlot(idx).type;
        //  InventoryManager
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
        inventoryUI_->close(); //  onInventoryClosed 
        //  onInventoryClosed 
        return;
    }

    // UI
    inventoryUI_ = InventoryUI::create(inventory_);
    if (!inventoryUI_) return;

    // 
    inventoryUI_->setCloseCallback([this]() {
        onInventoryClosed();
        });

    //  uiLayer_  Scene
    // uiLayer_  ZOrder  1000
    //  uiLayer_ InventoryUI 
    //  InventoryUI  TOP
    //  uiLayer_ 
    if (uiLayer_) {
         inventoryUI_->setPosition(Vec2::ZERO); // Local to uiLayer
         uiLayer_->addChild(inventoryUI_, 9999); 
    }
    else {
        // Fallback: Add to scene but won't follow camera without extra code
        this->addChild(inventoryUI_, 9999);
    }

    inventoryUI_->show();
}

void MineScene::onInventoryClosed()
{
    inventoryUI_ = nullptr;
    // 
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
    // 1. 
    if (!player_ || !miningManager_ || !mineLayer_ || !inventory_) return;

    // 2. 
    float energyPercent = player_->getCurrentEnergy() / player_->getMaxEnergy();
    if (energyPercent <= 0.2f)
    {
        showActionMessage("Exhausted!", Color3B::RED);
        return;
    }

    // 3.  ( Pickaxe)
    ItemType currentTool = inventory_->getSlot(selectedItemIndex_).type;
    if (currentTool != ItemType::Pickaxe)
    {
        showActionMessage("Need a Pickaxe!", Color3B::RED);
        handleAttackAction(); // 
        return;
    }

    // 4. 
    //  (Tile Coordinate)
    Vec2 playerPos = player_->getPosition();
    Vec2 playerTileCoord = mineLayer_->positionToTileCoord(playerPos);

    bool mined = false;

    //  ()
    float delay = 0.0f;
    if (energyPercent <= 0.5f) delay = 0.2f;

    //  ((0,0)8)
    std::vector<Vec2> offsets = {
        Vec2(0, 0),   // 
        Vec2(1, 0),   // 
        Vec2(-1, 0),  // 
        Vec2(0, 1),   //  (TiledY)
        Vec2(0, -1),  // 
        Vec2(1, 1),   // 
        Vec2(1, -1),  // 
        Vec2(-1, 1),  // 
        Vec2(-1, -1)  // 
    };

    for (const auto& offset : offsets)
    {
        // 
        Vec2 targetTile = playerTileCoord + offset;

        // 
        if (mineLayer_->isMineralAt(targetTile))
        {
            mined = true;

            // 
            player_->playSwingAnimation();

            //  ()
            if (delay > 0) {
                auto seq = Sequence::create(
                    DelayTime::create(delay),
                    CallFunc::create([this, targetTile]() {
                        executeMining(targetTile);
                        // 
                        if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
                             showActionMessage("Pickaxe broke!", Color3B::RED);
                             //  UI
                             if (inventoryUI_) inventoryUI_->refresh(); // Force refresh if open
                             initToolbarUI(); // Refresh toolbar
                        }
                        }),
                    nullptr
                );
                this->runAction(seq);
            }
            else {
                executeMining(targetTile);
                // 
                if (inventory_->decreaseDurability(selectedItemIndex_, 1)) {
                        showActionMessage("Pickaxe broke!", Color3B::RED);
                        if (inventoryUI_) inventoryUI_->refresh();
                        initToolbarUI();
                }
            }

            //  ( break)
            break;
        }
    }

    // 5. 
    if (!mined)
    {
        player_->playSwingAnimation();
    }
}


void MineScene::handleAttackAction()
{
    if (currentAttackCooldown_ > 0) return;

    // 
    player_->playSwingAnimation();
    currentAttackCooldown_ = attackCooldown_;

    Vec2 playerPos = player_->getPosition();
    float attackRange = 50.0f; // 
    int attackDamage = 1; // ??????
    ItemType item = ItemType::ITEM_NONE;
    bool shouldDecreaseDurability = false;

    // 
    if (inventory_)
    {
        item = inventory_->getSlot(selectedItemIndex_).type;
        shouldDecreaseDurability = InventoryManager::isTool(item);
        //  ( ItemType )
        //  Sword 
        if (item == ItemType::ITEM_WoodenSword || item == ItemType::ITEM_IronSword ||
            item == ItemType::ITEM_GoldSword || item == ItemType::ITEM_DiamondSword)
        {
            //  Weapon 
            attackDamage = Weapon::getWeaponAttackPower(item);
            attackRange = Weapon::getWeaponAttackRange(item);
        }
        else if (item == ItemType::Pickaxe || item == ItemType::Axe || item == ItemType::Scythe)
        {
            // 
            attackDamage = 3;
            attackRange = 40.0f;
        }
        else
        {
            // 
            // J
            // 
            // 1
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

            // 
            Vec2 knockback = monster->getPosition() - playerPos;
            knockback.normalize();
            monster->setPosition(monster->getPosition() + knockback * 30.0f);
        }
    }

    if (hit)
    {
        showActionMessage("Hit!", Color3B::ORANGE);
        if (inventory_ && shouldDecreaseDurability)
        {
            if (inventory_->decreaseDurability(selectedItemIndex_, 1))
            {
                showActionMessage("Weapon broke!", Color3B::RED);
                refreshToolbarUI();
            }
            if (inventoryUI_) inventoryUI_->refresh();
        }
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

                    // 
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
        // [Optimization 4] 
        if (elevatorSprite_)
        {
            float dist = player_->getPosition().distance(elevatorSprite_->getPosition());
            if (dist < 60.0f) // 
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
            //  (Fallback)
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

        // 
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
    CCLOG("Returning to farm...");
    bool saveSynced = syncSaveInventoryAndSkills(inventory_);
    auto gameScene = GameScene::createScene(saveSynced);
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, gameScene));
}

void MineScene::goToPreviousFloor()
{
    int prevFloor = currentFloor_ - 1;

    //  1-5 
    if (prevFloor < 1)
    {
        prevFloor = 5;  //  1  Q  5 
    }

    CCLOG("Switching to previous floor: %d -> %d", currentFloor_, prevFloor);
    auto prevScene = MineScene::createScene(inventory_, prevFloor);
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, prevScene));

}

void MineScene::goToNextFloor()
{
    int nextFloor = currentFloor_ + 1;

    //  1-5 
    if (nextFloor > 5)
    {
        nextFloor = 1;  //  5  E  1 
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

    // 
    if (pos.distance(player_->getPosition()) < 200.0f) return;

    Monster* monster = nullptr;

    // 
    int roll = rand() % 100;

    // 
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
    // 
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

    //  5 
    if (currentFloor_ % 5 != 0) return;

    //  ( 50%)
    if (rand() % 100 < 50) return;

    // 
    Vec2 pos = getRandomWalkablePosition();

    //  ()
    auto wellNode = DrawNode::create();
    wellNode->drawDot(Vec2::ZERO, 20, Color4F(0.2f, 0.4f, 1.0f, 0.8f)); // 
    wellNode->drawCircle(Vec2::ZERO, 22, 0, 30, false, Color4F::GRAY); // 

    wishingWell_ = Node::create();
    wishingWell_->setPosition(pos);
    wishingWell_->addChild(wellNode);
    this->addChild(wishingWell_, 5); // 

    // 
    auto label = Label::createWithSystemFont("Wishing Well\n(Press K)", "Arial", 12);
    label->setPosition(Vec2(0, 30));
    label->setAlignment(TextHAlignment::CENTER);
    wishingWell_->addChild(label);

    CCLOG("Wishing Well initialized at (%.1f, %.1f)", pos.x, pos.y);
}

void MineScene::handleWishAction()
{
    if (!player_ || !wishingWell_ || !inventory_) return;

    // 
    float dist = player_->getPosition().distance(wishingWell_->getPosition());
    if (dist > 60.0f)
    {
        showActionMessage("Too far from Wishing Well!", Color3B::GRAY);
        return;
    }

    // 
    ItemType currentItem = inventory_->getSlot(selectedItemIndex_).type;
    if (currentItem == ItemType::ITEM_NONE)
    {
        showActionMessage("Hold an item to wish!", Color3B::YELLOW);
        return;
    }

    // 
    if (inventory_->removeItem(currentItem, 1))
    {
        // 
        wishingWell_->runAction(Sequence::create(
            ScaleTo::create(0.1f, 1.2f),
            ScaleTo::create(0.1f, 1.0f),
            nullptr
        ));

        // 
        int randVal = rand() % 100;
        if (randVal < 30) // 30% 
        {
            showActionMessage("The well is silent...", Color3B::GRAY);
        }
        else if (randVal < 70) // 40%  / 
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
        else // 30% 
        {
            // 
            ItemType rewards[] = { ItemType::GoldOre, ItemType::ITEM_DiamondSword, ItemType::ITEM_GoldSword };
            ItemType reward = rewards[rand() % 3];

            // 
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
                    //
                    int gold = Weapon::getWeaponPrice(reward);
                    inventory_->addMoney(gold);
                    showActionMessage("Bag full, took Gold instead.", Color3B::ORANGE);
                }
            }
        }

        // UI
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

        //  uiLayer_ 
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
