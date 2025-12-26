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

    if (!loadTMXMap(tmxFile))
    {
        return false;
    }

    initCollisionLayer();
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

    // 查找底图层（支持英文 "Ground" 和中文 "图块层 1"）
    baseLayer_ = tmxMap_->getLayer("Ground");
    if (!baseLayer_)
    {
        baseLayer_ = tmxMap_->getLayer("图块层 1");
    }
    if (!baseLayer_)
    {
        // 如果都没找到，使用第一个 TMXLayer 作为底图
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

    return true;
}

void MapLayer::initCollisionLayer()
{
    if (!tmxMap_)
        return;

    // 查找 "Collision" 层（农场地图）
    collisionLayer_ = tmxMap_->getLayer("Collision");
    if (collisionLayer_)
    {
        collisionLayer_->setVisible(false);
        return;
    }

    // 查找 "Buildings" 层（矿洞地图）
    collisionLayer_ = tmxMap_->getLayer("Buildings");
    if (collisionLayer_)
    {
        collisionLayer_->setVisible(false);
        return;
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
        return true;  // 没有碰撞层则默认可走

    Vec2 tileCoord = positionToTileCoord(position);
    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return false;
    }

    int tileGID = collisionLayer_->getTileGIDAt(tileCoord);
    return (tileGID == 0);
}

bool MapLayer::hasCollisionAt(const Vec2& tileCoord) const
{
    if (!tmxMap_ || !collisionLayer_)
        return false;

    Size mapSize = tmxMap_->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return false;
    }

    int tileGID = collisionLayer_->getTileGIDAt(tileCoord);
    return tileGID != 0;
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
