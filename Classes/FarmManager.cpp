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

std::string FarmManager::getCropTextureName(int cropId, int stage) const  //类似于之前的lambda
{
    std::string name;

    // 根据 ID 查表 (对应你在 initCropDefs 里定义的 ID)
    switch (cropId)
    {
    case 0: name = "turnip";     break; // ID 0
    case 1: name = "potato";     break; // ID 1
    case 2: name = "corn";       break; // ID 2
    case 3: name = "strawberry"; break; // ★ ID 3 原番茄改为草莓
    case 4: name = "pumpkin";    break; // ID 4
    case 5: name = "blueberry";  break; // ID 5
    default: name = "turnip";    break;
    }

    // 你的图片是 1,2,3,4 (turnip1.png)
    // 程序的阶段是 0,1,2,3 (stage)
    // 所以要 +1
    int imgNum = stage + 1;

    // 拼接路径： "crops/" + 名字 + 数字 + ".png"
    return StringUtils::format("crops/%s%d.png", name.c_str(), imgNum);
}

bool FarmManager::init(MapLayer* mapLayer)
{
    if (!Node::init())
        return false;

    mapLayer_ = mapLayer;
    dayTimer_ = 30.0f; // Start at 6:00 AM (120s = 24h -> 30s = 6h)
    secondsPerDay_ = 120.0f; // 120 seconds = 1 in-game day
    dayCount_ = 1;

    if (mapLayer_)
    {
        mapSizeTiles_ = mapLayer_->getMapSizeInTiles();
        tileSize_ = mapLayer_->getTileSize();
    }

    initCropDefs();

    tiles_.resize(static_cast<size_t>(mapSizeTiles_.width * mapSizeTiles_.height));

    // 1. 泥土层 (overlay_)：画褐色方块，Z序为 5
    overlay_ = DrawNode::create();
    this->addChild(overlay_, 5);

    // 2. 作物层 (cropLayer_)：放作物图片，Z序为 10 (必须比泥土高，才能盖住泥土)
    cropLayer_ = Node::create();
    this->addChild(cropLayer_, 10);

    this->scheduleUpdate();
    redrawOverlay();

    CCLOG("FarmManager initialized. Map tiles: %.0fx%.0f, tile size: %.0fx%.0f",
        mapSizeTiles_.width, mapSizeTiles_.height, tileSize_.width, tileSize_.height);
    return true;
}

void FarmManager::initCropDefs()
{
    crops_.clear();

    // 6 种作物：简单周期与售价
    crops_[0] = {0, "Turnip", {1, 1, 1}, 60};
    crops_[1] = {1, "Potato", {1, 2, 2}, 80};
    crops_[2] = { 2, "Corn", {2, 2, 4}, 120 };
    crops_[3] = {3, "Tomato", {1, 2, 2}, 90};
    crops_[4] = {4, "Pumpkin", {2, 3, 3}, 180};
    crops_[5] = {5, "Blueberry", {1, 2, 2}, 110};
}

void FarmManager::update(float delta)
{
    dayTimer_ += delta;
    if (dayTimer_ >= secondsPerDay_)
    {
        dayTimer_ -= secondsPerDay_; // Keep overflow for precision
        progressDay();
    }
}

int FarmManager::getHour() const
{
    float totalMinutes = (dayTimer_ / secondsPerDay_) * 24.0f * 60.0f;
    return (int)(totalMinutes / 60.0f) % 24;
}

int FarmManager::getMinute() const
{
    float totalMinutes = (dayTimer_ / secondsPerDay_) * 24.0f * 60.0f;
    return (int)totalMinutes % 60;
}

FarmManager::ActionResult FarmManager::tillTile(const Vec2& tileCoord)
{
    ActionResult result{false, "", -1};
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

FarmManager::ActionResult FarmManager::plantSeed(const Vec2& tileCoord, int cropId)
{
    ActionResult result{false, "", -1};
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

    CropDef def = getCropDef(cropId);

    tile.hasCrop = true;
    tile.cropId = def.id;
    tile.stage = 0;
    tile.progressDays = 0;
    tile.watered = false;

    result.success = true;
    result.message = StringUtils::format("Planted %s", def.name.c_str());
    redrawOverlay();
    return result;
}

FarmManager::ActionResult FarmManager::waterTile(const Vec2& tileCoord)
{
    ActionResult result{false, "", -1};
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
    ActionResult result{false, "", -1};
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
    result.cropId = tile.cropId;
    tile.hasCrop = false;
    tile.cropId = -1;
    tile.stage = 0;
    tile.progressDays = 0;
    tile.watered = false;

    result.success = true;
    result.message = StringUtils::format("Harvested %s", def.name.c_str());
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

bool FarmManager::isTileClearForPlacement(const Vec2& tileCoord) const
{
    if (!isValidTile(tileCoord)) return false;

    // 检查是否有作物或已耕地
    auto& tile = tiles_[static_cast<size_t>(tileCoord.y * mapSizeTiles_.width + tileCoord.x)];
    if (tile.hasCrop || tile.tilled) return false;

    // 检查地图层碰撞 (建筑、水塘等)
    if (mapLayer_ && mapLayer_->hasCollisionAt(tileCoord)) return false;

    // 检查是否已有箱子
    if (getStorageChestAt(tileCoord)) return false;

    return true;
}

void FarmManager::addStorageChest(StorageChest* chest)
{
    if (!chest) return;
    
    // 检查是否已存在（避免重复添加）
    for (auto existing : storageChests_) {
        if (existing == chest) return;
    }

    storageChests_.push_back(chest);
    
    // 设置 Z-order 确保在地面之上
    this->addChild(chest, 20); 
    
    // 确保坐标对齐到格子中心
    Vec2 pos = mapLayer_->tileCoordToPosition(chest->getTileCoord());
    chest->setPosition(pos);
    
    CCLOG("Storage chest added at (%.0f, %.0f)", chest->getTileCoord().x, chest->getTileCoord().y);
}

StorageChest* FarmManager::getStorageChestAt(const Vec2& tileCoord) const
{
    for (auto chest : storageChests_) {
        if (chest->getTileCoord().fuzzyEquals(tileCoord, 0.1f)) {
            return chest;
        }
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

void FarmManager::setAllTiles(const std::vector<FarmTile>& tiles)
{
    if (tiles.size() != tiles_.size())
    {
        CCLOG("Warning: Loaded tile count (%zu) doesn't match current map size (%zu)",
            tiles.size(), tiles_.size());
        // 可以选择调整大小或只复制匹配的部分
        tiles_.resize(tiles.size());
    }
    tiles_ = tiles;
    redrawOverlay();
    CCLOG("Farm tiles loaded from save data");
}

void FarmManager::redrawOverlay()
{
    // 1. 清理上一帧的画面
    overlay_->clear();
    cropLayer_->removeAllChildren(); // ★ 把旧的作物全删了，准备根据最新数据重新贴

    if (!mapLayer_) return;

    float halfW = tileSize_.width / 2.0f;
    float halfH = tileSize_.height / 2.0f;

    // 遍历所有格子
    for (int y = 0; y < mapSizeTiles_.height; ++y)
    {
        for (int x = 0; x < mapSizeTiles_.width; ++x)
        {
            // 获取格子数据
            const auto& tile = tiles_[static_cast<size_t>(y * mapSizeTiles_.width + x)];

            // 如果没锄地，直接跳过
            if (!tile.tilled) continue;

            // 计算屏幕坐标
            Vec2 tileCoord(static_cast<float>(x), static_cast<float>(y));
            Vec2 center = mapLayer_->tileCoordToPosition(tileCoord);

            // --- A. 绘制地皮 (保持原逻辑) ---
            // 用代码画一个褐色的矩形代表耕地
            Vec2 bl(center.x - halfW + 1.5f, center.y - halfH + 1.5f);
            Vec2 tr(center.x + halfW - 1.5f, center.y + halfH - 1.5f);

            overlay_->drawSolidRect(bl, tr, kTilledColor);

            // 如果浇水了，叠一层蓝色
            if (tile.watered) {
                overlay_->drawSolidRect(bl, tr, Color4F(0.0f, 0.0f, 0.5f, 0.3f));
            }

            // --- B. 绘制作物 (使用 PNG) ---
            if (tile.hasCrop)
            {
                // 1. 拿到图片名 (例如 crops/turnip1.png)
                std::string filename = getCropTextureName(tile.cropId, tile.stage);

                // 2. 创建图片精灵
                auto sprite = Sprite::create(filename);
                if (sprite)
                {
                    // 【关键步骤 1】关闭抗锯齿，保证放大后像素清晰锐利，不会变模糊
                    sprite->getTexture()->setAliasTexParameters();

                    // 【关键步骤 2】获取图片原始大小
                    Size imgSize = sprite->getContentSize();

                    // 【关键步骤 3】根据尺寸判断缩放逻辑
                    // 如果高度小于等于 16 (即 16x16 的矮作物)，放大 2 倍变成 32x32
                    if (imgSize.height <= 16.0f)
                    {
                        sprite->setScale(2.0f);
                    }
                    // 如果高度已经是 32 (即 16x32 的高作物)，保持不变
                    else
                    {
                        sprite->setScale(1.0f);
                    }

                    // 设置位置
                    sprite->setPosition(center);

                    // 添加到层级
                    cropLayer_->addChild(sprite);
                }
            }
        }
    }
}