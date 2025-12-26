#include "BeachScene.h"
#include "MapLayer.h"
#include "Player.h"
#include "InventoryUI.h"
#include "InventoryManager.h"
#include "GameScene.h"
#include "HouseScene.h"
#include "FishingLayer.h"
#include "SkillManager.h"
#include "EnergyBar.h"
#include <algorithm>

USING_NS_CC;

namespace
{
    const char* kBeachMapFile = "map/beachMap.tmx";
    const Vec2 kBeachSpawnTile(3.0f, 10.0f);
    const int kBeachExitTileX = 1;
    const float kExitCooldownSeconds = 0.8f;
    const float kBeachPlayerScale = 1.0f;

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

    Vec2 findBeachSpawnTile(MapLayer* mapLayer, const Vec2& preferredTile)
    {
        if (!mapLayer)
            return preferredTile;

        auto tmxMap = mapLayer->getTMXMap();
        if (!tmxMap)
            return preferredTile;

        auto waterLayer = tmxMap->getLayer("Water");
        if (!waterLayer)
            return preferredTile;

        Size mapSize = tmxMap->getMapSize();
        int maxX = static_cast<int>(mapSize.width) - 1;
        int maxY = static_cast<int>(mapSize.height) - 1;
        if (maxX < 0 || maxY < 0)
            return preferredTile;

        int startY = std::min(std::max(static_cast<int>(preferredTile.y), 0), maxY);

        auto findLandToRightOfWater = [&](int row, Vec2& outTile) -> bool {
            int leftmostWater = -1;
            for (int x = 0; x <= maxX; ++x)
            {
                if (waterLayer->getTileGIDAt(Vec2(static_cast<float>(x), static_cast<float>(row))) != 0)
                {
                    leftmostWater = x;
                    break;
                }
            }

            if (leftmostWater < 0)
                return false;

            int candidateX = leftmostWater;
            while (candidateX <= maxX &&
                waterLayer->getTileGIDAt(Vec2(static_cast<float>(candidateX), static_cast<float>(row))) != 0)
            {
                ++candidateX;
            }

            if (candidateX > maxX)
                return false;

            outTile = Vec2(static_cast<float>(candidateX), static_cast<float>(row));
            return true;
        };

        Vec2 candidate;
        for (int offset = 0; offset <= maxY; ++offset)
        {
            int rowUp = startY + offset;
            if (rowUp <= maxY && findLandToRightOfWater(rowUp, candidate))
                return candidate;

            int rowDown = startY - offset;
            if (offset > 0 && rowDown >= 0 && findLandToRightOfWater(rowDown, candidate))
                return candidate;
        }

        return preferredTile;
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

BeachScene* BeachScene::createScene(InventoryManager* inventory, int dayCount, float accumulatedSeconds)
{
    BeachScene* ret = new (std::nothrow) BeachScene();
    if (ret && ret->init(inventory, dayCount, accumulatedSeconds))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BeachScene::init(InventoryManager* inventory, int dayCount, float accumulatedSeconds)
{
    if (!Scene::init())
        return false;

    inventory_ = inventory;
    dayCount_ = std::max(1, dayCount);
    accumulatedSeconds_ = std::max(0.0f, accumulatedSeconds);
    exitCooldown_ = kExitCooldownSeconds;

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

void BeachScene::initMap()
{
    mapLayer_ = MapLayer::create(kBeachMapFile);
    if (mapLayer_)
    {
        addChild(mapLayer_, 0);
    }
    else
    {
        CCLOG("ERROR: Failed to load beach map: %s", kBeachMapFile);
    }
}

void BeachScene::initPlayer()
{
    player_ = Player::create();
    if (!player_)
        return;

    if (mapLayer_)
    {
        Vec2 spawnTile = findBeachSpawnTile(mapLayer_, kBeachSpawnTile);
        spawnTile = findNearestWalkableTile(mapLayer_, spawnTile);
        Vec2 spawnPos = mapLayer_->tileCoordToPosition(spawnTile);
        player_->setPosition(spawnPos);
        mapLayer_->addChild(player_, 10);
        player_->setMapLayer(mapLayer_);
    }
    else
    {
        addChild(player_, 10);
    }

    player_->setScale(kBeachPlayerScale);
    player_->enableKeyboardControl();
}

void BeachScene::initCamera()
{
    auto camera = getDefaultCamera();
    if (camera && player_)
    {
        Vec2 playerPos = player_->getPosition();
        camera->setPosition(playerPos.x, playerPos.y);
    }
}

void BeachScene::initUI()
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

    auto helpLabel = Label::createWithSystemFont("B: Bag | ESC: Back to Farm", "Arial", 14);
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
}

void BeachScene::initControls()
{
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = CC_CALLBACK_2(BeachScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseDown = CC_CALLBACK_1(BeachScene::onMouseDown, this);
    auto mouseUpListener = EventListenerMouse::create();
    mouseUpListener->onMouseUp = CC_CALLBACK_1(BeachScene::onMouseUp, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseUpListener, this);
}

void BeachScene::initToolbar()
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
            toolbarItems_.push_back(ItemType::None);
        }
        selectedItemIndex_ = 0;
    }

    selectItemByIndex(selectedItemIndex_);
}

void BeachScene::initToolbarUI()
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

void BeachScene::refreshToolbarUI()
{
    if (toolbarSlots_.empty())
        return;

    if (inventory_)
    {
        int maxSlots = std::min(static_cast<int>(toolbarItems_.size()), inventory_->getSlotCount());
        for (int i = 0; i < maxSlots; ++i)
        {
            const auto& slot = inventory_->getSlot(i);
            ItemType newType = slot.isEmpty() ? ItemType::None : slot.type;

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

void BeachScene::selectItemByIndex(int idx)
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

void BeachScene::update(float delta)
{
    updateCamera();
    updateUI();
    updateFishingState(delta);

    auto energyBar = getChildByName("EnergyBar");
    auto camera = getDefaultCamera();
    if (energyBar && camera)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        energyBar->setPosition(Vec2(camera->getPositionX() + visibleSize.width / 2 - 50,
            camera->getPositionY() - visibleSize.height / 2 + 110));
    }

    accumulatedSeconds_ += delta;
    if (accumulatedSeconds_ >= secondsPerDay_)
    {
        if (inventory_) inventory_->removeMoney(200);
        showActionMessage("Passed out...", Color3B::RED);
        Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HouseScene::createScene(true)));
        return;
    }

    if (exitCooldown_ > 0.0f)
    {
        exitCooldown_ -= delta;
    }
    else if (!transitioning_ && isPlayerAtFarmExit())
    {
        backToFarm();
    }
}

void BeachScene::updateCamera()
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

void BeachScene::updateUI()
{
    if (timeLabel_)
    {
        float progress = accumulatedSeconds_ / secondsPerDay_;
        progress = clampf(progress, 0.0f, 0.999f);
        int totalMinutes = static_cast<int>(progress * 24.0f * 60.0f);
        int hour = totalMinutes / 60;
        int minute = totalMinutes % 60;
        timeLabel_->setString(StringUtils::format("Day %d, %02d:%02d", dayCount_, hour, minute));
    }

    if (moneyLabel_ && inventory_)
    {
        moneyLabel_->setString(StringUtils::format("Gold: %d", inventory_->getMoney()));
    }

    refreshToolbarUI();
}

void BeachScene::toggleInventory()
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
    }
    else
    {
        addChild(inventoryUI_, 2000);
    }

    inventoryUI_->show();
}

void BeachScene::onInventoryClosed()
{
    inventoryUI_ = nullptr;
    refreshToolbarUI();
}

void BeachScene::backToFarm()
{
    if (transitioning_)
        return;

    transitioning_ = true;
    auto gameScene = GameScene::createScene(true);
    Director::getInstance()->replaceScene(TransitionFade::create(0.8f, gameScene));
}

bool BeachScene::isPlayerAtFarmExit() const
{
    if (!player_ || !mapLayer_)
        return false;

    Vec2 tileCoord = mapLayer_->positionToTileCoord(player_->getPosition());
    return tileCoord.x <= kBeachExitTileX;
}

void BeachScene::showActionMessage(const std::string& text, const Color3B& color)
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

void BeachScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_ESCAPE:
        backToFarm();
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

void BeachScene::onMouseDown(Event* event)
{
    auto mouseEvent = static_cast<EventMouse*>(event);
    if (!mouseEvent || mouseEvent->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT)
        return;

    if (toolbarItems_.empty())
        return;

    ItemType current = ItemType::Hoe;
    if (selectedItemIndex_ >= 0 && selectedItemIndex_ < static_cast<int>(toolbarItems_.size()))
    {
        current = toolbarItems_[selectedItemIndex_];
    }

    if (current != ItemType::FishingRod)
        return;

    if (fishingState_ == FishingState::NONE)
    {
        if (isFishing_) return;
        if (player_ && player_->isFishingAnimationPlaying()) return;
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
        if (exclamationMark_) exclamationMark_->setVisible(false);
        if (player_) player_->startFishingReel();
        showActionMessage("Too early!", Color3B::RED);
    }
}

void BeachScene::onMouseUp(Event* event)
{
    auto mouseEvent = static_cast<EventMouse*>(event);
    if (!mouseEvent || mouseEvent->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT)
        return;

    if (fishingState_ == FishingState::CHARGING)
    {
        fishingState_ = FishingState::WAITING;
        if (chargeBarBg_) chargeBarBg_->setVisible(false);

        waitTimer_ = CCRANDOM_0_1() * 3.0f + 1.0f;
        if (player_) player_->startFishingCast();
        CCLOG("Casting rod! Power: %.2f. Waiting...", chargePower_);
    }
}

void BeachScene::updateFishingState(float delta)
{
    if (fishingState_ == FishingState::CHARGING)
    {
        chargePower_ += delta * 1.5f;
        if (chargePower_ > 1.0f) chargePower_ = 1.0f;

        if (chargeBarBg_) chargeBarBg_->setVisible(true);
        if (chargeBarFg_)
        {
            chargeBarFg_->setTextureRect(Rect(0, 0, 50 * chargePower_, 8));
            chargeBarFg_->setColor(Color3B(255 * chargePower_, 255 * (1 - chargePower_) + 100, 0));
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
            if (player_) player_->startFishingReel();
            CCLOG("Missed...");
            showActionMessage("Missed...", Color3B::GRAY);
        }
    }
}

void BeachScene::startFishing()
{
    isFishing_ = true;
    fishingState_ = FishingState::REELING;

    if (player_)
    {
        player_->setMoveSpeed(0.0f);
        player_->startFishingWait();
    }

    auto fishingLayer = FishingLayer::create();
    fishingLayer->setFinishCallback([this](bool success) {
        auto finish = [this, success]() {
            this->isFishing_ = false;
            this->fishingState_ = FishingState::NONE;

            if (this->chargeBarBg_) this->chargeBarBg_->setVisible(false);
            if (this->exclamationMark_) this->exclamationMark_->setVisible(false);
            if (this->player_) this->player_->setMoveSpeed(150.0f);

            if (success)
            {
                CCLOG("Fishing SUCCESS!");
                showActionMessage("Caught a Fish!", Color3B(255, 215, 0));
                SkillManager::getInstance()->recordAction(SkillManager::SkillType::Fishing);
            }
            else
            {
                CCLOG("Fishing FAILED.");
                showActionMessage("Fish got away...", Color3B::RED);
            }
        };

        if (this->player_)
        {
            this->player_->startFishingReel(finish);
        }
        else
        {
            finish();
        }
    });

    this->addChild(fishingLayer, 100);
}
