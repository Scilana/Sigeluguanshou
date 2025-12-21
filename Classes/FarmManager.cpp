#include "FarmManager.h"
#include "MapLayer.h"

USING_NS_CC;

namespace
{
Color4F kTilledColor(0.55f, 0.35f, 0.22f, 0.55f);
Color4F kWaterColor(0.25f, 0.45f, 0.85f, 0.35f);
Color4F kCropColor(0.25f, 0.8f, 0.35f, 0.85f);
Color4F kMatureColor(0.9f, 0.8f, 0.2f, 0.95f);
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

bool FarmManager::init(MapLayer* mapLayer)
{
    if (!Node::init())
        return false;

    mapLayer_ = mapLayer;
    dayTimer_ = 0.0f;
    secondsPerDay_ = 5.0f; // 5 seconds = 1 in-game day (for fast testing)
    dayCount_ = 1;

    if (mapLayer_)
    {
        mapSizeTiles_ = mapLayer_->getMapSizeInTiles();
        tileSize_ = mapLayer_->getTileSize();
    }

    initCropDefs();

    tiles_.resize(static_cast<size_t>(mapSizeTiles_.width * mapSizeTiles_.height));

    overlay_ = DrawNode::create();
    this->addChild(overlay_, 1);

    this->scheduleUpdate();
    redrawOverlay();

    CCLOG("FarmManager initialized. Map tiles: %.0fx%.0f, tile size: %.0fx%.0f",
        mapSizeTiles_.width, mapSizeTiles_.height, tileSize_.width, tileSize_.height);
    return true;
}

void FarmManager::initCropDefs()
{
    crops_.clear();

    // Simple default crop: Turnip, 3 stages, 1 day per stage.
    CropDef turnip;
    turnip.id = 0;
    turnip.name = "Turnip";
    turnip.stageDays = {1, 1, 1};
    turnip.salePrice = 60;
    crops_[turnip.id] = turnip;
}

void FarmManager::update(float delta)
{
    dayTimer_ += delta;
    if (dayTimer_ >= secondsPerDay_)
    {
        dayTimer_ = 0.0f;
        progressDay();
    }
}

FarmManager::ActionResult FarmManager::tillTile(const Vec2& tileCoord)
{
    ActionResult result{false, ""};
    if (!isValidTile(tileCoord))
    {
        result.message = "Out of farm bounds";
        return result;
    }

    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (tile.hasCrop)
    {
        result.message = "Crop already here";
        return result;
    }

    tile.tilled = true;
    tile.watered = false;
    result.success = true;
    result.message = "Tile tilled";
    redrawOverlay();
    return result;
}

FarmManager::ActionResult FarmManager::plantSeed(const Vec2& tileCoord)
{
    ActionResult result{false, ""};
    if (!isValidTile(tileCoord))
    {
        result.message = "Out of farm bounds";
        return result;
    }

    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (!tile.tilled)
    {
        result.message = "Till first";
        return result;
    }
    if (tile.hasCrop)
    {
        result.message = "Crop already here";
        return result;
    }

    tile.hasCrop = true;
    tile.cropId = 0; // default crop
    tile.stage = 0;
    tile.progressDays = 0;
    tile.watered = false;

    result.success = true;
    result.message = "Planted turnip";
    redrawOverlay();
    return result;
}

FarmManager::ActionResult FarmManager::waterTile(const Vec2& tileCoord)
{
    ActionResult result{false, ""};
    if (!isValidTile(tileCoord))
    {
        result.message = "Out of farm bounds";
        return result;
    }

    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (!tile.tilled)
    {
        result.message = "Till first";
        return result;
    }

    tile.watered = true;
    result.success = true;
    result.message = "Watered";
    redrawOverlay();
    return result;
}

FarmManager::ActionResult FarmManager::harvestTile(const Vec2& tileCoord)
{
    ActionResult result{false, ""};
    if (!isValidTile(tileCoord))
    {
        result.message = "Out of farm bounds";
        return result;
    }

    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (!tile.hasCrop)
    {
        result.message = "No crop here";
        return result;
    }
    if (!isMature(tile))
    {
        result.message = "Not mature yet";
        return result;
    }

    CropDef def = getCropDef(tile.cropId);
    tile.hasCrop = false;
    tile.cropId = -1;
    tile.stage = 0;
    tile.progressDays = 0;
    tile.watered = false;

    result.success = true;
    result.message = StringUtils::format("Harvested %s (+%d gold)", def.name.c_str(), def.salePrice);
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
    if (it != crops_.end())
        return it->second;

    CropDef fallback;
    fallback.id = -1;
    fallback.name = "Unknown";
    fallback.stageDays = {1, 1, 1};
    fallback.salePrice = 0;
    return fallback;
}

bool FarmManager::isMature(const FarmTile& tile) const
{
    auto def = getCropDef(tile.cropId);
    return tile.stage >= static_cast<int>(def.stageDays.size());
}

void FarmManager::progressDay()
{
    dayCount_ += 1;

    for (auto& tile : tiles_)
    {
        if (!tile.hasCrop)
        {
            tile.watered = false;
            continue;
        }

        if (tile.watered)
        {
            tile.progressDays += 1;
            auto def = getCropDef(tile.cropId);
            if (tile.stage < static_cast<int>(def.stageDays.size()) &&
                tile.progressDays >= def.stageDays[tile.stage])
            {
                tile.stage += 1;
                tile.progressDays = 0;
            }
        }

        tile.watered = false; // reset daily
    }

    redrawOverlay();
    CCLOG("Day advanced to %d", dayCount_);
}

void FarmManager::forceRedraw()
{
    redrawOverlay();
}

void FarmManager::redrawOverlay()
{
    overlay_->clear();

    if (!mapLayer_)
        return;

    float halfW = tileSize_.width / 2.0f;
    float halfH = tileSize_.height / 2.0f;

    for (int y = 0; y < mapSizeTiles_.height; ++y)
    {
        for (int x = 0; x < mapSizeTiles_.width; ++x)
        {
            const auto& tile = tiles_[static_cast<size_t>(y * mapSizeTiles_.width + x)];
            if (!tile.tilled)
                continue;

            Vec2 tileCoord(static_cast<float>(x), static_cast<float>(y));
            Vec2 center = mapLayer_->tileCoordToPosition(tileCoord);
            Vec2 bl(center.x - halfW + 1.5f, center.y - halfH + 1.5f);
            Vec2 tr(center.x + halfW - 1.5f, center.y + halfH - 1.5f);

            // Base tilled quad
            overlay_->drawSolidRect(bl, tr, kTilledColor);

            // Water overlay
            if (tile.watered)
            {
                overlay_->drawSolidRect(bl + Vec2(2, 2), tr - Vec2(2, 2), kWaterColor);
            }

            // Crop overlay
            if (tile.hasCrop)
            {
                Color4F cropColor = isMature(tile) ? kMatureColor : kCropColor;
                Vec2 cropInset(6, 6);
                overlay_->drawSolidRect(bl + cropInset, tr - cropInset, cropColor);

                if (isMature(tile))
                {
                    overlay_->drawRect(bl + Vec2(1, 1), tr - Vec2(1, 1), Color4F(1, 1, 0, 0.9f));
                }
            }
        }
    }
}

