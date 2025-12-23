#include "MineScene.h"
#include "GameScene.h"
#include "Slime.h"
#include "Zombie.h"
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
    monsterSpawnTimer_ = 0.0f;
    currentWeapon_ = ItemType::None;
    attackCooldown_ = 0.5f;
    currentAttackCooldown_ = 0.0f;

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
    initMonsters();
    initMonsters();
    initChests();
    initToolbar(); // 初始化工具栏

    // 启动更新
    this->scheduleUpdate();

    CCLOG("Mine Scene initialized successfully!");
    return true;
}

void MineScene::initMap()
{
    CCLOG("Initializing mine map...");

    CCLOG("Initializing mine map...");

    // 根据楼层加载不同地图
    // 限制在1-4层
    int mapIndex = currentFloor_;
    if (mapIndex > 4) mapIndex = 4;
    if (mapIndex < 1) mapIndex = 1;
    
    std::string mapFile = StringUtils::format("map/mine_floor%d.tmx", mapIndex);

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
        // 设置玩家初始位置（矿洞入口/楼梯处）
        // 假设楼梯在地图上方或特定位置
        // 这里简单设置为地图中心偏下
        if (mineLayer_)
        {
            Size mapSize = mineLayer_->getMapSize();
            Vec2 startPos = Vec2(mapSize.width / 2, mapSize.height - 150);
            
            // 如果该位置有碰撞，尝试找附近位置
            if (!mineLayer_->isWalkable(startPos))
            {
                startPos = getRandomWalkablePosition();
            }

            player_->setPosition(startPos);
        }
        else
        {
            auto visibleSize = Director::getInstance()->getVisibleSize();
            player_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
        }

        player_->enableKeyboardControl();
        if (mineLayer_)
        {
            player_->setMapLayer(mineLayer_);
        }

        this->addChild(player_, 10);
    }
    else
    {
        CCLOG("ERROR: Failed to create player!");
    }
}

void MineScene::initCamera()
{
    auto camera = this->getDefaultCamera();
    if (camera)
    {
        Vec2 playerPos = player_->getPosition();
        camera->setPosition(playerPos.x, playerPos.y);
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

    // 当前物品
    itemLabel_ = Label::createWithSystemFont("Tool: None", "Arial", 18);
    itemLabel_->setAnchorPoint(Vec2(1, 0.5));
    itemLabel_->setPosition(Vec2(origin.x + visibleSize.width - 20, origin.y + visibleSize.height - 20));
    itemLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(itemLabel_, 1);

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

    // 初始生成一些怪物
    // 数量随楼层增加
    int initialCount = 2 + currentFloor_;
    if (initialCount > 10) initialCount = 10;

    for (int i = 0; i < initialCount; ++i)
    {
        spawnMonster();
    }
}

void MineScene::initChests()
{
    chests_.clear();

    // 初始生成宝箱
    // 概率随楼层增加
    int chestCount = 1;
    if (currentFloor_ >= 3) chestCount = 2 + rand() % 2;
    else if (currentFloor_ >= 2) chestCount = 1 + rand() % 2;
    
    // 如果运气好，多生成一个
    if (rand() % 100 < 20) chestCount++;

    for (int i = 0; i < chestCount; ++i)
    {
        Vec2 pos = getRandomWalkablePosition();
        
        auto chest = TreasureChest::create(currentFloor_);
        chest->setPosition(pos);
        this->addChild(chest, 5); // 宝箱层级
        
        chests_.push_back(chest);
    }
}

void MineScene::update(float delta)
{
    Scene::update(delta);
    updateCamera();
    updateUI();
    updateMonsters(delta);

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
        if (type == ItemType::None) name = "Empty";
        
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
    // 初始化工具栏物品 (使用 ID 0-9)
    toolbarItems_.clear();
    if (inventory_)
    {
        for (int i = 0; i < 10; ++i)
        {
            auto slot = inventory_->getSlot(i);
            toolbarItems_.push_back(slot.type);
        }
    }
    else
    {
        // Fallback if no inventory
        for (int i = 0; i < 10; ++i) toolbarItems_.push_back(ItemType::None);
    }
    
    selectedItemIndex_ = 0;
    
    // 如果有UI，选中默认
    this->selectItemByIndex(0);
}

void MineScene::selectItemByIndex(int idx)
{
    if (idx < 0 || idx >= 10) return;
    selectedItemIndex_ = idx;
    
    // 重新获取当前工具类型（因为背包可能变动）
    if (inventory_) {
        toolbarItems_[idx] = inventory_->getSlot(idx).type;
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
    } else {
        this->addChild(inventoryUI_, 2000);
    }
    
    inventoryUI_->show();
}

void MineScene::onInventoryClosed()
{
    inventoryUI_ = nullptr;
    // 背包关闭后，刷新工具栏数据
    initToolbar();
}

void MineScene::handleMiningAction()
{
    if (!player_ || !miningManager_ || !mineLayer_ || !inventory_) return;

    // 检查是否装备了镐子 (Pickaxe)
    ItemType currentTool = inventory_->getSlot(selectedItemIndex_).type;
    if (currentTool != ItemType::Pickaxe)
    {
        showActionMessage("Need a Pickaxe!", Color3B::RED);
        // 如果没镐子，尝试当作攻击处理
        handleAttackAction();
        return;
    }

    // 获取玩家位置和朝向
    Vec2 playerPos = player_->getPosition();
    Vec2 facingDir = player_->getFacingDirection();
    
    // 归一化朝向并稍微延伸一点距离，以确定面前的格子
    Vec2 targetPos = playerPos + facingDir * 32.0f;
    
    // 将位置转换为瓦片坐标
    Vec2 tileCoord = mineLayer_->positionToTileCoord(targetPos);
    
    // 尝试挖矿（精准挖掘）
    bool mined = false;
    
    // 优先检测面向的格子
    if (mineLayer_->isMineralAt(tileCoord))
    {
        MiningManager::MiningResult result = miningManager_->mineTile(tileCoord);
        if (result.success)
        {
            mined = true;
            showActionMessage(result.message, Color3B::GREEN);
        }
    }
    
    // 脚下检测
    if (!mined)
    {
        Vec2 footTile = mineLayer_->positionToTileCoord(playerPos);
        if (mineLayer_->isMineralAt(footTile))
        {
            MiningManager::MiningResult result = miningManager_->mineTile(footTile);
            if (result.success)
            {
                mined = true;
                showActionMessage(result.message, Color3B::GREEN);
            }
        }
    }
    
    if (!mined)
    {
        // 即使有镐子但没挖到矿，也可以挥动一下（攻击）
        handleAttackAction();
    }
}


void MineScene::handleAttackAction()
{
    if (currentAttackCooldown_ > 0) return;
    
    // 播放攻击动画
    // player_->playAttackAnimation();
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
        if (item == ItemType::WoodenSword || item == ItemType::IronSword || 
            item == ItemType::GoldSword || item == ItemType::DiamondSword)
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
            if (result.item != ItemType::None)
            {
                inventory_->addItem(result.item, result.count);
                showActionMessage(result.message, Color3B::YELLOW);
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
        // 显示电梯UI（如果需要）
        // 或者直接返回农场
        backToFarm();
        break;
        
    case EventKeyboard::KeyCode::KEY_TAB:
    case EventKeyboard::KeyCode::KEY_E:
    case EventKeyboard::KeyCode::KEY_I:
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
    case EventKeyboard::KeyCode::KEY_9: selectItemByIndex(8); break;
    case EventKeyboard::KeyCode::KEY_0: selectItemByIndex(9); break;
        
    default:
        break;
    }
}

void MineScene::backToFarm()
{
    auto gameScene = GameScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, gameScene));
}

void MineScene::goToNextFloor()
{
    int nextFloor = currentFloor_ + 1;
    auto nextScene = MineScene::createScene(inventory_, nextFloor);
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, nextScene));
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
    
    return Vec2(mapSize.width/2, mapSize.height/2);
}
