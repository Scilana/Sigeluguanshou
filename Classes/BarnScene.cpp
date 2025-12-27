#include "BarnScene.h"
#include "MapLayer.h"
#include "Player.h"
#include "InventoryManager.h"
#include "InventoryUI.h"
#include "TimeManager.h"
#include "HouseScene.h"
#include "EnergyBar.h"
#include <algorithm>
#include <sstream>

USING_NS_CC;

namespace
{
    const char* kBarnMapFile = "map/Barn.tmx";
    const Vec2 kFallbackDoorTile(11.0f, 14.0f);

    const float kToolbarSlotSize = 48.0f;
    const float kToolbarSlotPadding = 6.0f;
    const float kToolbarIconPadding = 6.0f;
    const Color4B kToolbarBarColor(40, 35, 30, 220);
    const Color3B kToolbarSlotColor(70, 60, 50);
    const Color3B kToolbarSlotSelectedColor(170, 150, 95);

    Vec2 findNearestWalkableTile(MapLayer* mapLayer, const Vec2& desiredTile)
    {
        if (!mapLayer)
            return desiredTile;

        Size mapSize = mapLayer->getMapSizeInTiles();
        int maxX = static_cast<int>(mapSize.width) - 1;
        int maxY = static_cast<int>(mapSize.height) - 1;
        if (maxX < 0 || maxY < 0)
            return desiredTile;

        int startX = std::min(std::max(static_cast<int>(desiredTile.x), 0), maxX);
        int startY = std::min(std::max(static_cast<int>(desiredTile.y), 0), maxY);

        auto isWalkableTile = [mapLayer](int x, int y) {
            Vec2 pos = mapLayer->tileCoordToPosition(Vec2(static_cast<float>(x), static_cast<float>(y)));
            return mapLayer->isWalkable(pos);
        };

        if (isWalkableTile(startX, startY))
        {
            return Vec2(static_cast<float>(startX), static_cast<float>(startY));
        }

        int maxRadius = static_cast<int>(std::max(mapSize.width, mapSize.height));
        for (int r = 1; r <= maxRadius; ++r)
        {
            for (int dy = -r; dy <= r; ++dy)
            {
                for (int dx = -r; dx <= r; ++dx)
                {
                    if (dx != -r && dx != r && dy != -r && dy != r)
                        continue;

                    int x = startX + dx;
                    int y = startY + dy;
                    if (x < 0 || x > maxX || y < 0 || y > maxY)
                        continue;

                    if (isWalkableTile(x, y))
                    {
                        return Vec2(static_cast<float>(x), static_cast<float>(y));
                    }
                }
            }
        }

        return Vec2(static_cast<float>(startX), static_cast<float>(startY));
    }

    Vec2 getDoorTileFromWarp(MapLayer* mapLayer)
    {
        if (!mapLayer)
            return kFallbackDoorTile;

        auto tmxMap = mapLayer->getTMXMap();
        if (!tmxMap)
            return kFallbackDoorTile;

        auto props = tmxMap->getProperties();
        auto it = props.find("Warp");
        if (it == props.end())
            return kFallbackDoorTile;

        std::istringstream iss(it->second.asString());
        int x = 0;
        int y = 0;
        if (!(iss >> x >> y))
            return kFallbackDoorTile;

        Size mapSize = tmxMap->getMapSize();
        int maxX = static_cast<int>(mapSize.width) - 1;
        int maxY = static_cast<int>(mapSize.height) - 1;
        x = std::min(std::max(x, 0), maxX);
        y = std::min(std::max(y, 0), maxY);
        return Vec2(static_cast<float>(x), static_cast<float>(y));
    }

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

BarnScene* BarnScene::createScene()
{
    BarnScene* ret = new (std::nothrow) BarnScene();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BarnScene::init()
{
    if (!Scene::init())
        return false;

    inventory_ = InventoryManager::getInstance();

    initMap();
    initPlayer();
    initCamera();
    initUI();
    initControls();
    initToolbar();
    initToolbarUI();

    scheduleUpdate();
    return true;
}

void BarnScene::initMap()
{
    mapLayer_ = MapLayer::create(kBarnMapFile);
    if (mapLayer_)
    {
        addChild(mapLayer_, 0);
        auto tmxMap = mapLayer_->getTMXMap();
        if (tmxMap)
        {
            auto frontLayer = tmxMap->getLayer("Front");
            if (frontLayer)
            {
                frontLayer->setLocalZOrder(20);
            }
        }
    }
    else
    {
        CCLOG("ERROR: Failed to load barn map: %s", kBarnMapFile);
    }
}

void BarnScene::initPlayer()
{
    player_ = Player::create();
    if (!player_)
        return;

    if (mapLayer_)
    {
        player_->setMapLayer(mapLayer_);
        exitTile_ = getDoorTileFromWarp(mapLayer_);

        Size mapSize = mapLayer_->getMapSizeInTiles();
        if (exitTile_.y >= mapSize.height)
        {
            exitTile_.y = mapSize.height - 1;
        }

        Vec2 spawnTile(exitTile_.x, std::max(0.0f, exitTile_.y - 1.0f));
        spawnTile = findNearestWalkableTile(mapLayer_, spawnTile);
        player_->setPosition(mapLayer_->tileCoordToPosition(spawnTile));

        auto tileSize = mapLayer_->getTileSize();
        exitRadius_ = std::max(40.0f, std::max(tileSize.width, tileSize.height) * 2.0f);

        mapLayer_->addChild(player_, 10);
    }
    else
    {
        addChild(player_, 10);
    }

    player_->enableKeyboardControl();
}

void BarnScene::initCamera()
{
    auto camera = getDefaultCamera();
    if (camera && player_)
    {
        Vec2 playerPos = player_->getPosition();
        camera->setPosition(playerPos.x, playerPos.y);
    }
}

void BarnScene::initUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    uiLayer_ = Layer::create();
    addChild(uiLayer_, 1000);

    auto topBar = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, 60);
    topBar->setAnchorPoint(Vec2(0, 1));
    topBar->setPosition(Vec2(origin.x, origin.y + visibleSize.height));
    uiLayer_->addChild(topBar, 0);

    timeLabel_ = Label::createWithSystemFont("Day 1, 06:00", "Arial", 24);
    timeLabel_->setAnchorPoint(Vec2(0, 0.5f));
    timeLabel_->setPosition(Vec2(origin.x + 20.0f, origin.y + visibleSize.height - 30.0f));
    timeLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(timeLabel_, 1);

    moneyLabel_ = Label::createWithSystemFont("Gold: 0", "Arial", 24);
    moneyLabel_->setAnchorPoint(Vec2(1, 0.5f));
    moneyLabel_->setPosition(Vec2(origin.x + visibleSize.width - 20.0f, origin.y + visibleSize.height - 30.0f));
    moneyLabel_->setColor(Color3B(255, 215, 0));
    uiLayer_->addChild(moneyLabel_, 1);

    actionLabel_ = Label::createWithSystemFont("", "Arial", 20);
    actionLabel_->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 90));
    actionLabel_->setColor(Color3B::WHITE);
    uiLayer_->addChild(actionLabel_, 1);

    auto helpLabel = Label::createWithSystemFont("B: Bag | ENTER: Exit", "Arial", 14);
    helpLabel->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 20));
    helpLabel->setColor(Color3B(200, 200, 200));
    uiLayer_->addChild(helpLabel, 1);

    if (player_)
    {
        auto energyBar = EnergyBar::create(player_);
        if (energyBar)
        {
            energyBar->setName("EnergyBar");
            addChild(energyBar, 100);
        }
    }
}

void BarnScene::initControls()
{
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(BarnScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
}

void BarnScene::initToolbar()
{
    toolbarItems_.clear();
    if (inventory_)
    {
        for (int i = 0; i < 8; ++i)
        {
            toolbarItems_.push_back(inventory_->getSlot(i).type);
        }
        selectedItemIndex_ = inventory_->getSelectedSlotIndex();
        if (selectedItemIndex_ < 0 || selectedItemIndex_ >= static_cast<int>(toolbarItems_.size()))
        {
            selectedItemIndex_ = 0;
        }
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            toolbarItems_.push_back(ItemType::ITEM_NONE);
        }
        selectedItemIndex_ = 0;
    }

    selectItemByIndex(selectedItemIndex_);
}

void BarnScene::initToolbarUI()
{
    if (!uiLayer_ || toolbarItems_.empty())
        return;

    if (toolbarUI_)
    {
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
    toolbarUI_->setAnchorPoint(Vec2(0.5f, 0.0f));
    toolbarUI_->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 6.0f));
    uiLayer_->addChild(toolbarUI_, 10);

    toolbarSlots_.reserve(slotCount);
    toolbarIcons_.reserve(slotCount);
    toolbarCounts_.reserve(slotCount);
    toolbarCountCache_.assign(slotCount, -1);

    for (int i = 0; i < slotCount; ++i)
    {
        float x = kToolbarSlotPadding + i * (kToolbarSlotSize + kToolbarSlotPadding);
        auto slotBg = Sprite::create();
        slotBg->setTextureRect(Rect(0, 0, kToolbarSlotSize, kToolbarSlotSize));
        slotBg->setColor(kToolbarSlotColor);
        slotBg->setAnchorPoint(Vec2(0, 0));
        slotBg->setPosition(Vec2(x, kToolbarSlotPadding));
        toolbarUI_->addChild(slotBg);
        toolbarSlots_.push_back(slotBg);

        auto icon = createToolbarIcon(toolbarItems_[i]);
        if (icon)
        {
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

void BarnScene::refreshToolbarUI()
{
    if (toolbarSlots_.empty())
        return;

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

void BarnScene::selectItemByIndex(int idx)
{
    if (toolbarItems_.empty())
        return;

    if (idx < 0 || idx >= static_cast<int>(toolbarItems_.size()))
        return;

    selectedItemIndex_ = idx;

    if (inventory_)
    {
        toolbarItems_[idx] = inventory_->getSlot(idx).type;
        inventory_->setSelectedSlotIndex(idx);
    }

    ItemType currentItem = toolbarItems_[selectedItemIndex_];
    if (player_)
    {
        player_->setCurrentTool(currentItem);
    }

    refreshToolbarUI();
}

void BarnScene::update(float delta)
{
    auto tm = TimeManager::getInstance();
    if (tm)
    {
        tm->update(delta);
        if (tm->isMidnight())
        {
            if (inventory_) inventory_->removeMoney(200);
            showActionMessage("Passed out...", Color3B::RED);
            Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HouseScene::createScene(true)));
            return;
        }
    }

    updateCamera();
    updateUI();

    auto energyBar = getChildByName("EnergyBar");
    auto camera = getDefaultCamera();
    if (energyBar && camera)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        energyBar->setPosition(Vec2(camera->getPositionX() + visibleSize.width / 2 - 50,
            camera->getPositionY() - visibleSize.height / 2 + 110));
    }
}

void BarnScene::updateCamera()
{
    if (!player_)
        return;

    auto camera = getDefaultCamera();
    if (!camera)
        return;

    Vec2 playerPos = player_->getPosition();
    Vec3 currentPos = camera->getPosition3D();
    Vec3 targetPos = Vec3(playerPos.x, playerPos.y, currentPos.z);

    float smoothFactor = 0.1f;
    Vec3 newPos = currentPos + (targetPos - currentPos) * smoothFactor;

    if (mapLayer_)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        Size mapSize = mapLayer_->getMapSize();

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

    if (uiLayer_)
    {
        Vec2 uiPos = Vec2(
            newPos.x - Director::getInstance()->getVisibleSize().width / 2,
            newPos.y - Director::getInstance()->getVisibleSize().height / 2
        );
        uiLayer_->setPosition(uiPos);
    }
}

void BarnScene::updateUI()
{
    auto tm = TimeManager::getInstance();
    if (tm && timeLabel_)
    {
        timeLabel_->setString(StringUtils::format("Day %d, %02d:%02d",
            tm->getDay(),
            tm->getHour(),
            tm->getMinute()));
    }

    if (moneyLabel_ && inventory_)
    {
        moneyLabel_->setString(StringUtils::format("Gold: %d", inventory_->getMoney()));
    }

    refreshToolbarUI();
}

void BarnScene::toggleInventory()
{
    if (inventoryUI_)
    {
        inventoryUI_->close();
        return;
    }

    inventoryUI_ = InventoryUI::create(inventory_);
    if (!inventoryUI_)
        return;

    inventoryUI_->setCloseCallback([this]() {
        onInventoryClosed();
        });

    if (uiLayer_)
    {
        uiLayer_->addChild(inventoryUI_, 100);
        inventoryUI_->setPosition(Vec2::ZERO);
        inventoryUI_->show();
    }
    else
    {
        addChild(inventoryUI_, 100);
        inventoryUI_->show();
    }
}

void BarnScene::onInventoryClosed()
{
    inventoryUI_ = nullptr;
}

void BarnScene::showActionMessage(const std::string& text, const Color3B& color)
{
    if (!actionLabel_)
        return;

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

void BarnScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_ENTER:
    case EventKeyboard::KeyCode::KEY_KP_ENTER:
        exitBarn();
        break;
    case EventKeyboard::KeyCode::KEY_TAB:
    case EventKeyboard::KeyCode::KEY_B:
        toggleInventory();
        break;
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

bool BarnScene::isPlayerAtExit() const
{
    if (!player_ || !mapLayer_)
        return false;

    Vec2 doorPos = mapLayer_->tileCoordToPosition(exitTile_);
    return player_->getPosition().distance(doorPos) <= exitRadius_;
}

void BarnScene::exitBarn()
{
    if (!isPlayerAtExit())
    {
        showActionMessage("Door is too far!", Color3B::RED);
        return;
    }

    Director::getInstance()->popScene();
}
