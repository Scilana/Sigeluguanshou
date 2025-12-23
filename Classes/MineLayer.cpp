#include "MineLayer.h"

USING_NS_CC;

MineLayer* MineLayer::create(const std::string& tmxFile)
{
    MineLayer* ret = new (std::nothrow) MineLayer();
    if (ret && ret->init(tmxFile))
    {
        ret->autorelease();
        return ret;
    }
    else
    {
        delete ret;
        ret = nullptr;
        return nullptr;
    }
}

bool MineLayer::init(const std::string& tmxFile)
{
    // 调用父类的初始化（加载 TMX 地图）
    if (!MapLayer::init(tmxFile))
        return false;

    CCLOG("Initializing MineLayer with: %s", tmxFile.c_str());

    // 初始化矿洞特有的图层
    initMineLayers();

    return true;
}

void MineLayer::initMineLayers()
{
    auto tmxMap = getTMXMap();
    if (!tmxMap)
    {
        CCLOG("ERROR: TMX map is null in MineLayer");
        return;
    }

    // 矿物层
    mineralLayer_ = tmxMap->getLayer("mineral");
    if (!mineralLayer_)
    {
        CCLOG("WARNING: 'mineral' layer not found in mine map");
    }
    else
    {
        CCLOG("Mineral layer initialized");
    }

    // 楼梯层
    stairsLayer_ = tmxMap->getLayer("stairs");
    if (!stairsLayer_)
    {
        CCLOG("WARNING: 'stairs' layer not found in mine map");
    }
    else
    {
        CCLOG("Stairs layer initialized");
    }
}

bool MineLayer::isMineralAt(const Vec2& tileCoord) const
{
    if (!mineralLayer_)
        return false;

    int x = static_cast<int>(tileCoord.x);
    int y = static_cast<int>(tileCoord.y);

    Size mapSize = getMapSizeInTiles();
    if (x < 0 || y < 0 || x >= mapSize.width || y >= mapSize.height)
        return false;

    int gid = mineralLayer_->getTileGIDAt(Vec2(x, y));
    return gid != 0;
}

bool MineLayer::isStairsAt(const Vec2& tileCoord) const
{
    if (!stairsLayer_)
        return false;

    int x = static_cast<int>(tileCoord.x);
    int y = static_cast<int>(tileCoord.y);

    Size mapSize = getMapSizeInTiles();
    if (x < 0 || y < 0 || x >= mapSize.width || y >= mapSize.height)
        return false;

    int gid = stairsLayer_->getTileGIDAt(Vec2(x, y));
    return gid != 0;
}

void MineLayer::clearMineralAt(const Vec2& tileCoord)
{
    if (!mineralLayer_)
        return;

    int x = static_cast<int>(tileCoord.x);
    int y = static_cast<int>(tileCoord.y);

    mineralLayer_->setTileGID(0, Vec2(x, y));
}

int MineLayer::getMineralGID(const Vec2& tileCoord) const
{
    if (!mineralLayer_)
        return 0;

    int x = static_cast<int>(tileCoord.x);
    int y = static_cast<int>(tileCoord.y);

    return mineralLayer_->getTileGIDAt(Vec2(x, y));
}
