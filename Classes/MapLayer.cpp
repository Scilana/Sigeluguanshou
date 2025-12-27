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

    // 指针初始化
    tmxMap_ = nullptr;
    baseLayer_ = nullptr;
    collisionLayer_ = nullptr;
    waterLayer_ = nullptr;
    treeLayer_ = nullptr;

    if (!loadTMXMap(tmxFile))
    {
        CCLOG("Failed to load TMX map: %s", tmxFile.c_str());
        return false;
    }

    // 初始化碰撞层（调用下面定义的函数）
    initCollisionLayer();

    CCLOG("MapLayer initialized with map: %s", tmxFile.c_str());
    return true;
}

bool MapLayer::loadTMXMap(const std::string& tmxFile)
{
    // 1. 创建 TiledMap 对象
    tmxMap_ = TMXTiledMap::create(tmxFile);
    if (!tmxMap_)
    {
        CCLOG("Error: Cannot load TMX file: %s", tmxFile.c_str());
        return false;
    }

    // 2. 将地图添加到场景 (注意：只添加这一次)
    this->addChild(tmxMap_, 0);

    // 3. 获取 Ground 层 (底图)
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

    // 4. [新增] 获取 Tree 层
    treeLayer_ = tmxMap_->getLayer("Tree");
    if (!treeLayer_)
    {
        CCLOG("Warning: 'Tree' layer not found in TMX file!");
    }
    else
    {
        CCLOG("Tree layer loaded.");
    }

    // 5. 打印调试信息
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

    // 1. 尝试查找名为 "Collision" 的层
    collisionLayer_ = tmxMap_->getLayer("Collision");

    if (collisionLayer_)
    {
        //通常我们把碰撞层隐藏，不让玩家看到红色的块
        collisionLayer_->setVisible(false);
        CCLOG("Collision layer found and hidden");
    }
    else
    {
        CCLOG("Warning: 'Collision' layer not found in TMX!");
    }

    // 2. 顺便初始化水域层 (如果有的话)
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

    // 边界检查
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        // CCLOG("Position out of map bounds: tile(%.0f, %.0f)", tileCoord.x, tileCoord.y);
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
            return false; // Collision 层有 tile = 不可行走
        }
    }

    // ==========================================
    // 方式2：检查所有层的 tile 属性
    // ==========================================
    auto children = tmxMap_->getChildren();
    for (auto child : children)
    {
        auto layer = dynamic_cast<TMXLayer*>(child);
        if (layer && layer->isVisible())
        {
            if (layer == collisionLayer_) continue;

            int gid = layer->getTileGIDAt(tileCoord);
            if (gid > 0)
            {
                auto properties = tmxMap_->getPropertiesForGID(gid);
                if (!properties.isNull())
                {
                    auto propMap = properties.asValueMap();

                    // 检查 Collidable 属性（注意：TMX 中是大写 C）
                    if (propMap.find("Collidable") != propMap.end())
                    {
                        if (propMap["Collidable"].asBool()) return false;
                    }

                    // 兼容小写 collidable
                    if (propMap.find("collidable") != propMap.end())
                    {
                        if (propMap["collidable"].asBool()) return false;
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
            return false;
        }
    }

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

// =======================================================
// [新增] Tree 层专用接口
// =======================================================

int MapLayer::getTreeGIDAt(const cocos2d::Vec2& tileCoord) const
{
    if (!tmxMap_ || !treeLayer_) return 0;

    // 简单的越界检查
    Size s = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.y < 0 || tileCoord.x >= s.width || tileCoord.y >= s.height)
        return 0;

    return treeLayer_->getTileGIDAt(tileCoord);
}

void MapLayer::setTreeGID(const cocos2d::Vec2& tileCoord, int gid)
{
    if (!tmxMap_ || !treeLayer_) return;

    Size s = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.y < 0 || tileCoord.x >= s.width || tileCoord.y >= s.height)
        return;

    treeLayer_->setTileGID(gid, tileCoord);
}