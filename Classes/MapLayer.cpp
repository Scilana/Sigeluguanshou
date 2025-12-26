#include "MapLayer.h"

USING_NS_CC;

MapLayer* MapLayer::create(const std::string& tmxFile)
{
    MapLayer* layer = new (std::nothrow) MapLayer();
    if (layer && layer->init(tmxFile))
    {
        layer->autorelease();
        return layer;
    }
    CC_SAFE_DELETE(layer);
    return nullptr;
}

bool MapLayer::init(const std::string& tmxFile)
{
    if (!Layer::init())
        return false;

    tmxMap_ = nullptr;
    baseLayer_ = nullptr;
    collisionLayer_ = nullptr;
    waterLayer_ = nullptr;

    if (!loadTMXMap(tmxFile))
    {
        CCLOG("Failed to load TMX map: %s", tmxFile.c_str());
        return false;
    }

    initCollisionLayer();
    CCLOG("MapLayer initialized with map: %s", tmxFile.c_str());
    return true;
}

bool MapLayer::loadTMXMap(const std::string& tmxFile)
{
    tmxMap_ = TMXTiledMap::create(tmxFile);
    if (!tmxMap_)
    {
        CCLOG("Error: Cannot load TMX file: %s", tmxFile.c_str());
        return false;
    }

    // 记录第一层（底图），方便清除树贴图
    // [Fix] Renamed "图块层 1" to "Ground" to avoid encoding issues
    baseLayer_ = tmxMap_->getLayer("Ground");
    if (!baseLayer_)
    {
        CCLOG("DEBUG: 'Ground' layer not found. Listing all available layers:");
        for (const auto& child : tmxMap_->getChildren()) {
             auto layer = dynamic_cast<TMXLayer*>(child);
             if (layer) {
                 CCLOG(" - Found Layer: %s (Opacity: %d, Visible: %d)", layer->getLayerName().c_str(), layer->getOpacity(), layer->isVisible());
             }
        }

        // 如果没有固定名称，取第一个 TMXLayer 作为底图
        // 如果没有固定名称，取第一个 TMXLayer 作为底图
        auto children = tmxMap_->getChildren();
        for (auto child : children)
        {
            auto layer = dynamic_cast<TMXLayer*>(child);
            if (layer)
            {
                baseLayer_ = layer;
                break;
            }
        }
    }

    this->addChild(tmxMap_, 0);

    Size mapSize = tmxMap_->getMapSize();
    Size tileSize = tmxMap_->getTileSize();
    CCLOG("Map loaded: %s", tmxFile.c_str());
    CCLOG("  Map size: %.0f x %.0f tiles", mapSize.width, mapSize.height);
    CCLOG("  Tile size: %.0f x %.0f pixels", tileSize.width, tileSize.height);
    CCLOG("  Total size: %.0f x %.0f pixels",
        mapSize.width * tileSize.width,
        mapSize.height * tileSize.height);

    return true;
}

void MapLayer::initCollisionLayer()
{
    if (!tmxMap_)
        return;

    // 尝试查找 "Collision" 层（农场地图）
    collisionLayer_ = tmxMap_->getLayer("Collision");
    if (collisionLayer_)
    {
        collisionLayer_->setVisible(false);
        CCLOG("Collision layer found and hidden");
    }

    // 尝试查找 "Buildings" 层（矿洞地图）
    if (!collisionLayer_)
    {
        collisionLayer_ = tmxMap_->getLayer("Buildings");
        if (collisionLayer_)
        {
            collisionLayer_->setVisible(false);
            CCLOG("Buildings layer found and used as collision layer (hidden)");
        }
    }

    if (!collisionLayer_)
    {
        CCLOG("Warning: No collision layer found (tried 'Collision' and 'Buildings')");
    }

    waterLayer_ = tmxMap_->getLayer("Water");
}


Size MapLayer::getMapSize() const
{
    if (!tmxMap_)
        return Size::ZERO;

    Size mapSizeInTiles = tmxMap_->getMapSize();
    Size tileSize = tmxMap_->getTileSize();
    return Size(
        mapSizeInTiles.width * tileSize.width,
        mapSizeInTiles.height * tileSize.height
    );
}

Size MapLayer::getMapSizeInTiles() const
{
    if (!tmxMap_)
        return Size::ZERO;
    return tmxMap_->getMapSize();
}

Size MapLayer::getTileSize() const
{
    if (!tmxMap_)
        return Size::ZERO;
    return tmxMap_->getTileSize();
}

bool MapLayer::isWalkable(const Vec2& position) const
{
    if (!tmxMap_)
    {
        CCLOG("Warning: tmxMap_ is null in isWalkable");
        return true;
    }

    // 将世界坐标转换为 tile 坐标
    Vec2 tileCoord = positionToTileCoord(position);
    Size mapSize = tmxMap_->getMapSize();

    // 调试日志（可选，上线后可删除）
    // CCLOG("Checking walkable: world(%.1f, %.1f) -> tile(%.0f, %.0f)", 
    //       position.x, position.y, tileCoord.x, tileCoord.y);

    // 边界检查
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        CCLOG("Position out of map bounds: tile(%.0f, %.0f)", tileCoord.x, tileCoord.y);
        return false;
    }

    // ==========================================
    // 方式1：检查专门的 Collision 层
    // ==========================================
    if (collisionLayer_)
    {
        int tileGID = collisionLayer_->getTileGIDAt(tileCoord);
        if (tileGID != 0)
        {
            // 调试日志
            // CCLOG("Blocked by Collision layer at tile(%.0f, %.0f), GID=%d", 
            //       tileCoord.x, tileCoord.y, tileGID);
            return false; // Collision 层有 tile = 不可行走
        }
    }

    // ==========================================
    // 方式2：检查所有层的 tile 属性
    // ==========================================
    // 遍历所有可见层，检查是否有带 Collidable 属性的 tile
    auto children = tmxMap_->getChildren();
    for (auto child : children)
    {
        auto layer = dynamic_cast<TMXLayer*>(child);
        if (layer && layer->isVisible())
        {
            // 跳过 Collision 层（已经在方式1中检查过了）
            if (layer == collisionLayer_)
                continue;

            int gid = layer->getTileGIDAt(tileCoord);
            if (gid > 0)
            {
                // 获取该 tile 的属性
                auto properties = tmxMap_->getPropertiesForGID(gid);
                if (!properties.isNull())
                {
                    auto propMap = properties.asValueMap();

                    // 检查 Collidable 属性（注意：TMX 中是大写 C）
                    if (propMap.find("Collidable") != propMap.end())
                    {
                        bool isCollidable = propMap["Collidable"].asBool();
                        if (isCollidable)
                        {
                            // 调试日志
                            CCLOG("Blocked by Collidable tile in layer '%s' at tile(%.0f, %.0f), GID=%d",
                                layer->getLayerName().c_str(), tileCoord.x, tileCoord.y, gid);
                            return false;
                        }
                    }

                    // 兼容小写 collidable（可选）
                    if (propMap.find("collidable") != propMap.end())
                    {
                        bool isCollidable = propMap["collidable"].asBool();
                        if (isCollidable)
                        {
                            CCLOG("Blocked by collidable tile in layer '%s' at tile(%.0f, %.0f), GID=%d",
                                layer->getLayerName().c_str(), tileCoord.x, tileCoord.y, gid);
                            return false;
                        }
                    }
                }
            }
        }
    }

    // ==========================================
    // 方式3：检查 Water 层（水域）
    // ==========================================
    if (waterLayer_)
    {
        int waterGID = waterLayer_->getTileGIDAt(tileCoord);
        if (waterGID != 0)
        {
            // 调试日志
            // CCLOG("Blocked by Water layer at tile(%.0f, %.0f), GID=%d", 
            //       tileCoord.x, tileCoord.y, waterGID);
            return false;
        }
    }

    // 所有检查都通过，可以行走
    return true;
}


bool MapLayer::hasCollisionAt(const Vec2& tileCoord) const
{
    if (!tmxMap_)
        return false;

    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return false;
    }

    if (collisionLayer_)
    {
        int tileGID = collisionLayer_->getTileGIDAt(tileCoord);
        if (tileGID != 0)
            return true;
    }

    if (waterLayer_)
    {
        int waterGID = waterLayer_->getTileGIDAt(tileCoord);
        if (waterGID != 0)
            return true;
    }

    return false;
}


void MapLayer::clearCollisionAt(const Vec2& tileCoord)
{
    if (!tmxMap_ || !collisionLayer_)
        return;

    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return;
    }

    collisionLayer_->setTileGID(0, tileCoord);
}

void MapLayer::clearBaseTileAt(const Vec2& tileCoord)
{
    if (!tmxMap_ || !baseLayer_)
        return;

    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return;
    }

    baseLayer_->setTileGID(0, tileCoord);
}

int MapLayer::getBaseTileGID(const Vec2& tileCoord) const
{
    if (!tmxMap_ || !baseLayer_)
        return 0;

    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return 0;
    }

    return baseLayer_->getTileGIDAt(tileCoord);
}

void MapLayer::setBaseTileGID(const Vec2& tileCoord, int gid)
{
    if (!tmxMap_ || !baseLayer_)
        return;

    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return;
    }

    baseLayer_->setTileGID(gid, tileCoord);
}

Vec2 MapLayer::positionToTileCoord(const Vec2& position) const
{
    if (!tmxMap_)
        return Vec2::ZERO;

    Size tileSize = tmxMap_->getTileSize();
    Size mapSize = tmxMap_->getMapSize();

    int x = static_cast<int>(position.x / tileSize.width);
    int y = static_cast<int>((mapSize.height * tileSize.height - position.y) / tileSize.height);
    return Vec2(static_cast<float>(x), static_cast<float>(y));
}

Vec2 MapLayer::tileCoordToPosition(const Vec2& tileCoord) const
{
    if (!tmxMap_)
        return Vec2::ZERO;

    Size tileSize = tmxMap_->getTileSize();
    Size mapSize = tmxMap_->getMapSize();

    float x = tileCoord.x * tileSize.width + tileSize.width / 2;
    float y = (mapSize.height - tileCoord.y) * tileSize.height - tileSize.height / 2;
    return Vec2(x, y);
}
