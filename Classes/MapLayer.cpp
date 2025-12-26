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
        CCLOG("Failed to load TMX map: %s", tmxFile.c_str());
        return false;
    }

    initCollisionLayer();
    CCLOG("MapLayer initialized with map: %s", tmxFile.c_str());
    return true;
}

bool MapLayer::loadTMXMap(const std::string& tmxFile)
{
    CCLOG("========================================");
    CCLOG("MapLayer: Loading TMX file: %s", tmxFile.c_str());

    // 清除纹理缓存，强制重新加载所有图块集图片
    CCLOG("MapLayer: Clearing texture cache to force reload...");
    Director::getInstance()->getTextureCache()->removeUnusedTextures();

    tmxMap_ = TMXTiledMap::create(tmxFile);
    if (!tmxMap_)
    {
        CCLOG("ERROR: Cannot load TMX file: %s", tmxFile.c_str());
        return false;
    }

    CCLOG("MapLayer: TMX map created successfully");

    // 查找底图层（支持多种名称）
    // 优先尝试新的英文名称 "Ground"
    baseLayer_ = tmxMap_->getLayer("Ground");
    if (baseLayer_)
    {
        CCLOG("MapLayer: 'Ground' layer found successfully");
    }
    else
    {
        // 尝试旧的中文名称 "图块层 1"
        baseLayer_ = tmxMap_->getLayer("图块层 1");
        if (baseLayer_)
        {
            CCLOG("MapLayer: '图块层 1' layer found successfully");
        }
        else
        {
            // 如果都没找到，使用第一个 TMXLayer 作为底图
            CCLOG("MapLayer: Neither 'Ground' nor '图块层 1' found, searching for first TMXLayer...");
            auto children = tmxMap_->getChildren();
            CCLOG("MapLayer: TMX map has %d children", (int)children.size());
            for (auto child : children)
            {
                auto layer = dynamic_cast<TMXLayer*>(child);
                if (layer)
                {
                    baseLayer_ = layer;
                    CCLOG("MapLayer: Using first TMXLayer as baseLayer: %s", layer->getLayerName().c_str());
                    break;
                }
            }
        }
    }

    if (!baseLayer_)
    {
        CCLOG("ERROR: No baseLayer found at all!");
    }

    // 添加地图到场景
    this->addChild(tmxMap_, 0);
    CCLOG("MapLayer: TMX map added to MapLayer as child");

    // 输出地图信息
    Size mapSize = tmxMap_->getMapSize();
    Size tileSize = tmxMap_->getTileSize();
    CCLOG("Map loaded: %s", tmxFile.c_str());
    CCLOG("  Map size: %.0f x %.0f tiles", mapSize.width, mapSize.height);
    CCLOG("  Tile size: %.0f x %.0f pixels", tileSize.width, tileSize.height);
    CCLOG("  Total size: %.0f x %.0f pixels",
        mapSize.width * tileSize.width,
        mapSize.height * tileSize.height);

    // 列出所有图层及其可见性
    auto children = tmxMap_->getChildren();
    CCLOG("  Layers in map:");
    for (auto child : children)
    {
        auto layer = dynamic_cast<TMXLayer*>(child);
        if (layer)
        {
            CCLOG("    - %s (visible: %d, opacity: %d)",
                layer->getLayerName().c_str(),
                layer->isVisible() ? 1 : 0,
                layer->getOpacity());

            // 输出该层使用的纹理信息
            auto texture = layer->getTexture();
            if (texture)
            {
                CCLOG("      Texture: %s (size: %.0fx%.0f)",
                    texture->getPath().c_str(),
                    texture->getContentSize().width,
                    texture->getContentSize().height);
            }
            else
            {
                CCLOG("      WARNING: Layer has no texture!");
            }
        }
    }

    CCLOG("========================================");
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
        return;
    }

    // 尝试查找 "Buildings" 层（矿洞地图）
    collisionLayer_ = tmxMap_->getLayer("Buildings");
    if (collisionLayer_)
    {
        collisionLayer_->setVisible(false);
        CCLOG("Buildings layer found and used as collision layer (hidden)");
        return;
    }

    CCLOG("Warning: No collision layer found (tried 'Collision' and 'Buildings')");
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
