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

    // 加载TMX地图
    if (!loadTMXMap(tmxFile))
    {
        CCLOG("Failed to load TMX map: %s", tmxFile.c_str());
        return false;
    }

    // 初始化碰撞层
    initCollisionLayer();

    CCLOG("MapLayer initialized with map: %s", tmxFile.c_str());
    return true;
}

bool MapLayer::loadTMXMap(const std::string& tmxFile)
{
    // 加载TMX地图
    tmxMap_ = TMXTiledMap::create(tmxFile);
    if (!tmxMap_)
    {
        CCLOG("Error: Cannot load TMX file: %s", tmxFile.c_str());
        return false;
    }

    // 添加到层
    this->addChild(tmxMap_, 0);

    // 打印地图信息
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

    // 尝试获取名为 "Collision" 的图层
    collisionLayer_ = tmxMap_->getLayer("Collision");

    if (collisionLayer_)
    {
        // 隐藏碰撞层（不显示）
        collisionLayer_->setVisible(false);
        CCLOG("Collision layer found and hidden");
    }
    else
    {
        CCLOG("Warning: No collision layer found (layer name should be 'Collision')");
    }
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
    if (!tmxMap_ || !collisionLayer_)
        return true;  // 如果没有碰撞层，默认可行走

    // 转换为瓦片坐标
    Vec2 tileCoord = positionToTileCoord(position);

    // 检查是否超出地图范围
    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return false;  // 超出地图范围，不可行走
    }

    // 获取碰撞层的瓦片GID
    // GID > 0 表示有瓦片（不可行走）
    // GID = 0 表示没有瓦片（可行走）
    int tileGID = collisionLayer_->getTileGIDAt(tileCoord);

    return (tileGID == 0);  // GID为0表示可行走
}

Vec2 MapLayer::positionToTileCoord(const Vec2& position) const
{
    if (!tmxMap_)
        return Vec2::ZERO;

    Size tileSize = tmxMap_->getTileSize();
    Size mapSize = tmxMap_->getMapSize();

    // 计算瓦片坐标
    int x = (int)(position.x / tileSize.width);
    int y = (int)((mapSize.height * tileSize.height - position.y) / tileSize.height);

    return Vec2(x, y);
}

Vec2 MapLayer::tileCoordToPosition(const Vec2& tileCoord) const
{
    if (!tmxMap_)
        return Vec2::ZERO;

    Size tileSize = tmxMap_->getTileSize();
    Size mapSize = tmxMap_->getMapSize();

    // 计算世界坐标（瓦片中心）
    float x = tileCoord.x * tileSize.width + tileSize.width / 2;
    float y = (mapSize.height - tileCoord.y) * tileSize.height - tileSize.height / 2;

    return Vec2(x, y);
}