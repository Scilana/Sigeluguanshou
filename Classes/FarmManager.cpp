#include "FarmManager.h"
#include "MapLayer.h"
#include "TimeManager.h"
#include "SaveManager.h"

USING_NS_CC;

namespace
{
    Color4F kTilledColor(0.55f, 0.35f, 0.22f, 0.55f);
    Color4F kWaterColor(0.25f, 0.45f, 0.85f, 0.35f);
} // namespace

FarmManager* FarmManager::create(MapLayer* mapLayer)
{
    FarmManager* mgr = new (std::nothrow) FarmManager();
    if (mgr && mgr->init(mapLayer))
    {
        mgr->autorelease();
        return mgr;
    }
    CC_SAFE_DELETE(mgr);
    return nullptr;
}

std::string FarmManager::getCropTextureName(int cropId, int stage) const
{
    std::string name;
    switch (cropId)
    {
    case 0: name = "turnip";     break;
    case 1: name = "potato";     break;
    case 2: name = "corn";       break;
    case 3: name = "strawberry"; break;
    case 4: name = "pumpkin";    break;
    case 5: name = "blueberry";  break;
    default: name = "turnip";    break;
    }
    int imgNum = stage + 1;
    return StringUtils::format("crops/%s%d.png", name.c_str(), imgNum);
}

bool FarmManager::init(MapLayer* mapLayer)
{
    if (!Node::init()) return false;
    mapLayer_ = mapLayer;
    if (mapLayer_)
    {
        mapSizeTiles_ = mapLayer_->getMapSizeInTiles();
        tileSize_ = mapLayer_->getTileSize();
    }
    initCropDefs();
    tiles_.resize(static_cast<size_t>(mapSizeTiles_.width * mapSizeTiles_.height));
    overlay_ = DrawNode::create();
    this->addChild(overlay_, 5);
    cropLayer_ = Node::create();
    this->addChild(cropLayer_, 10);

    Vec2 binPos(24, 15);
    shippingBin_ = ShippingBin::create(binPos);
    if (shippingBin_) {
        this->addChild(shippingBin_, 15);
        storageChests_.push_back(shippingBin_);
    }

    this->scheduleUpdate();
    
    // Attempt to load saved farm state
    // This is crucial when reloading the scene (e.g. after sleeping/passing out)
    SaveManager::SaveData saveData;
    if (SaveManager::getInstance()->hasSaveFile() && SaveManager::getInstance()->loadGame(saveData))
    {
        CCLOG("FarmManager: Loading saved tiles...");
        for (const auto& tileData : saveData.farmTiles)
        {
            if (isValidTile(Vec2(tileData.x, tileData.y)))
            {
               int idx = tileData.y * mapSizeTiles_.width + tileData.x;
               auto& tile = tiles_[idx];
               tile.tilled = tileData.tilled;
               tile.watered = tileData.watered;
               tile.hasCrop = tileData.hasCrop;
               tile.cropId = tileData.cropId;
               tile.stage = tileData.stage;
               tile.progressDays = tileData.progressDays;
            }
        }
    }

    redrawOverlay();
    return true;
}

void FarmManager::initCropDefs()
{
    crops_.clear();
    crops_[0] = {0, "Turnip", {1, 1, 1}, 60};
    crops_[1] = {1, "Potato", {1, 2, 2}, 80};
    crops_[2] = { 2, "Corn", {2, 2, 4}, 120 };
    crops_[3] = {3, "Tomato", {1, 2, 2}, 90};
    crops_[4] = {4, "Pumpkin", {2, 3, 3}, 180};
    crops_[5] = {5, "Blueberry", {1, 2, 2}, 110};
}

void FarmManager::progressDay()
{
    // Update TimeManager's record first
    auto tm = TimeManager::getInstance();
    if (tm) {
        tm->setLastFarmUpdateDay(tm->getDay());
    }

    if (shippingBin_ && priceFunction_) {
        int totalEarnings = 0;
        auto inv = shippingBin_->getInventory();
        auto& slots = inv->getAllSlots();
        for (const auto& slot : slots) {
            if (!slot.isEmpty()) {
                int price = priceFunction_(slot.type);
                if (price > 0) totalEarnings += price * slot.count;
            }
        }
        inv->clear();
        if (totalEarnings > 0 && earningsCallback_) earningsCallback_(totalEarnings);
    }

    for (auto& tile : tiles_)
    {
        if (tile.hasCrop && tile.watered)
        {
            auto def = getCropDef(tile.cropId);
            if (tile.stage < (int)def.stageDays.size())
            {
                tile.progressDays++;
                if (tile.progressDays >= def.stageDays[tile.stage])
                {
                    tile.stage++;
                    tile.progressDays = 0;
                }
            }
        }
        tile.watered = false;
    }
    redrawOverlay();
}

void FarmManager::update(float delta)
{
    // Catch-up logic for saved games or scene transitions
    auto tm = TimeManager::getInstance();
    if (tm) {
        int currentDay = tm->getDay();
        int lastUpdate = tm->getLastFarmUpdateDay();
        
        // If this is the very first run (lastUpdate == 0), just sync it
        if (lastUpdate == 0) {
            tm->setLastFarmUpdateDay(currentDay);
        }
            // Missed some days! Catch up.
            int daysMissed = currentDay - lastUpdate;
            CCLOG("FarmManager catching up: %d days", daysMissed);
            
            // Prevent recursive updates to LastFarmUpdateDay inside the loop
            // We just want to run the logic N times.
            for (int i = 0; i < daysMissed; ++i) {
                // Manually run the logic usually found in progressDay
                // But without the TimeManager update part which ruins the loop
                
                // 1. Shipping Bin
                if (shippingBin_ && priceFunction_) {
                    int totalEarnings = 0;
                    auto inv = shippingBin_->getInventory();
                    auto& slots = inv->getAllSlots();
                    for (const auto& slot : slots) {
                        if (!slot.isEmpty()) {
                            int price = priceFunction_(slot.type);
                            if (price > 0) totalEarnings += price * slot.count;
                        }
                    }
                    inv->clear();
                    if (totalEarnings > 0 && earningsCallback_) earningsCallback_(totalEarnings);
                }

                // 2. Crop Growth
                for (auto& tile : tiles_)
                {
                    if (tile.hasCrop && tile.watered)
                    {
                        auto def = getCropDef(tile.cropId);
                        if (tile.stage < (int)def.stageDays.size())
                        {
                            tile.progressDays++;
                            if (tile.progressDays >= def.stageDays[tile.stage])
                            {
                                tile.stage++;
                                tile.progressDays = 0;
                            }
                        }
                    }
                    tile.watered = false; // Reset water daily
                }
            }
            
            // Finally update the timestamp to current
            tm->setLastFarmUpdateDay(currentDay);
            redrawOverlay();
    }
}

int FarmManager::getHour() const { return TimeManager::getInstance()->getHour(); }
int FarmManager::getMinute() const { return TimeManager::getInstance()->getMinute(); }

FarmManager::ActionResult FarmManager::tillTile(const Vec2& tileCoord)
{
    ActionResult result{false, "", -1};
    if (!isValidTile(tileCoord)) return result;
    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (tile.hasCrop) return result;
    tile.tilled = true;
    tile.watered = false;
    result.success = true;
    redrawOverlay();
    return result;
}

FarmManager::ActionResult FarmManager::plantSeed(const Vec2& tileCoord, int cropId)
{
    ActionResult result{false, "", -1};
    if (!isValidTile(tileCoord)) return result;
    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (!tile.tilled || tile.hasCrop) return result;
    CropDef def = getCropDef(cropId);
    tile.hasCrop = true;
    tile.cropId = def.id;
    tile.stage = 0;
    tile.progressDays = 0;
    tile.watered = false;
    result.success = true;
    redrawOverlay();
    return result;
}

FarmManager::ActionResult FarmManager::waterTile(const Vec2& tileCoord)
{
    ActionResult result{false, "", -1};
    if (!isValidTile(tileCoord)) return result;
    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (!tile.tilled) return result;
    tile.watered = true;
    result.success = true;
    redrawOverlay();
    return result;
}

FarmManager::ActionResult FarmManager::harvestTile(const Vec2& tileCoord)
{
    ActionResult result{false, "", -1};
    if (!isValidTile(tileCoord)) return result;
    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (!tile.hasCrop || !isMature(tile)) return result;
    result.cropId = tile.cropId;
    tile.hasCrop = false;
    tile.cropId = -1;
    tile.stage = 0;
    tile.progressDays = 0;
    tile.watered = false;
    result.success = true;
    redrawOverlay();
    return result;
}

bool FarmManager::isValidTile(const Vec2& tileCoord) const
{
    return tileCoord.x >= 0 && tileCoord.y >= 0 &&
        tileCoord.x < mapSizeTiles_.width && tileCoord.y < mapSizeTiles_.height;
}

FarmManager::CropDef FarmManager::getCropDef(int cropId) const
{
    auto it = crops_.find(cropId);
    if (it != crops_.end()) return it->second;
    CropDef fallback{-1, "Unknown", {1, 1, 1}, 0};
    return fallback;
}

bool FarmManager::isMature(const FarmTile& tile) const
{
    auto def = getCropDef(tile.cropId);
    return tile.stage >= static_cast<int>(def.stageDays.size());
}

bool FarmManager::isTileClearForPlacement(const Vec2& tileCoord) const
{
    if (!isValidTile(tileCoord)) return false;
    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (tile.hasCrop || tile.tilled) return false;
    if (mapLayer_ && mapLayer_->hasCollisionAt(tileCoord)) return false;
    if (getStorageChestAt(tileCoord)) return false;
    return true;
}

void FarmManager::addStorageChest(StorageChest* chest)
{
    if (!chest) return;
    for (auto existing : storageChests_) if (existing == chest) return;
    storageChests_.push_back(chest);
    this->addChild(chest, 20);
    Vec2 pos = mapLayer_->tileCoordToPosition(chest->getTileCoord());
    chest->setPosition(pos);
}

StorageChest* FarmManager::getStorageChestAt(const Vec2& tileCoord) const
{
    for (auto chest : storageChests_) {
        if (chest->getTileCoord().fuzzyEquals(tileCoord, 0.1f)) return chest;
    }
    return nullptr;
}

void FarmManager::removeStorageChest(StorageChest* chest)
{
    auto it = std::find(storageChests_.begin(), storageChests_.end(), chest);
    if (it != storageChests_.end()) {
        storageChests_.erase(it);
        chest->removeFromParent();
    }
}

void FarmManager::forceRedraw() { redrawOverlay(); }

void FarmManager::setAllTiles(const std::vector<FarmTile>& tiles)
{
    if (tiles.size() != tiles_.size()) tiles_.resize(tiles.size());
    tiles_ = tiles;
    redrawOverlay();
}

void FarmManager::redrawOverlay()
{
    overlay_->clear();
    cropLayer_->removeAllChildren();
    if (!mapLayer_) return;
    float halfW = tileSize_.width / 2.0f;
    float halfH = tileSize_.height / 2.0f;
    for (int y = 0; y < mapSizeTiles_.height; ++y)
    {
        for (int x = 0; x < mapSizeTiles_.width; ++x)
        {
            const auto& tile = tiles_[static_cast<size_t>(y * mapSizeTiles_.width + x)];
            if (!tile.tilled) continue;
            Vec2 tileCoord(static_cast<float>(x), static_cast<float>(y));
            Vec2 center = mapLayer_->tileCoordToPosition(tileCoord);
            auto soilSprite = Sprite::create("soil.png");
            if (soilSprite)
            {
                soilSprite->getTexture()->setAliasTexParameters();
                soilSprite->setScale(soilSprite->getContentSize().height <= 16.0f ? 2.0f : 1.0f);
                soilSprite->setPosition(center);
                if (tile.watered) soilSprite->setColor(Color3B(180, 180, 255));
                cropLayer_->addChild(soilSprite, 0);
            }
            else
            {
                Vec2 bl(center.x - halfW + 1.5f, center.y - halfH + 1.5f);
                Vec2 tr(center.x + halfW - 1.5f, center.y + halfH - 1.5f);
                overlay_->drawSolidRect(bl, tr, kTilledColor);
                if (tile.watered) overlay_->drawSolidRect(bl, tr, Color4F(0.0f, 0.0f, 0.5f, 0.3f));
            }
            if (tile.hasCrop)
            {
                auto sprite = Sprite::create(getCropTextureName(tile.cropId, tile.stage));
                if (sprite)
                {
                    sprite->getTexture()->setAliasTexParameters();
                    sprite->setScale(sprite->getContentSize().height <= 16.0f ? 2.0f : 1.0f);
                    sprite->setPosition(center);
                    cropLayer_->addChild(sprite, 1);
                }
            }
        }
    }
}

int FarmManager::getDayCount() const { return TimeManager::getInstance()->getDay(); }
void FarmManager::setDayCount(int dayCount) { TimeManager::getInstance()->setDayCount(dayCount); }
float FarmManager::getDayProgress() const
{
    auto tm = TimeManager::getInstance();
    return tm ? tm->getDayProgress() : 0.0f;
}