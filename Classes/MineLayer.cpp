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
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool MineLayer::init(const std::string& tmxFile)
{
    // 调用父类的 init 来加载 TMX 地图
    if (!MapLayer::init(tmxFile))
    {
        return false;
    }

    mineralLayer_ = nullptr;
    stairsLayer_ = nullptr;

    CCLOG("MineLayer initialized with: %s", tmxFile.c_str());

    // 新的矿洞地图使用 "Buildings" 层作为碰撞层
    auto tmxMap = getTMXMap();
    if (tmxMap)
    {
        mineralLayer_ = tmxMap->getLayer("mineral");
        if (!mineralLayer_)
            mineralLayer_ = tmxMap->getLayer("Mineral");
        if (!mineralLayer_)
            mineralLayer_ = tmxMap->getLayer("mine1");
        
        if (mineralLayer_) CCLOG("Found mineral layer: %s", mineralLayer_->getLayerName().c_str());
        
        stairsLayer_ = tmxMap->getLayer("stairs");
        if (!stairsLayer_)
            stairsLayer_ = tmxMap->getLayer("Stairs");
        if (stairsLayer_) CCLOG("Found stairs layer: %s", stairsLayer_->getLayerName().c_str());


        // 检查图层
        auto backLayer = tmxMap->getLayer("Back");
        auto buildingsLayer = tmxMap->getLayer("Buildings");
        auto frontLayer = tmxMap->getLayer("Front");

        if (backLayer) CCLOG("Found Back layer");
        if (buildingsLayer) CCLOG("Found Buildings layer (collision)");
        if (frontLayer) CCLOG("Found Front layer");
    }

    return true;
}

bool MineLayer::isMineralAt(const Vec2& tileCoord) const
{
    if (!mineralLayer_)
        return false;

    Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
        return false;

    return mineralLayer_->getTileGIDAt(tileCoord) != 0;
}

int MineLayer::getMineralGID(const Vec2& tileCoord) const
{
    if (!mineralLayer_)
        return 0;

    Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
        return 0;

    return mineralLayer_->getTileGIDAt(tileCoord);
}

void MineLayer::clearMineralAt(const Vec2& tileCoord)
{
    if (!mineralLayer_)
        return;

    Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
        return;

    mineralLayer_->setTileGID(0, tileCoord);
}

bool MineLayer::isStairsAt(const Vec2& tileCoord) const
{
    if (!stairsLayer_)
        return false;

    Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
        return false;

    return stairsLayer_->getTileGIDAt(tileCoord) != 0;
}

bool MineLayer::isWalkable(const Vec2& position) const
{
    // 1. 检查障碍物 (Buildings 层，通过父类 MapLayer 检查)
    if (!MapLayer::isWalkable(position)) return false;

    // 2. 检查地面 (Back 层)
    // 只有 Back 层有图块的地方才是地面，没有图块的地方是虚空/墙壁
    auto tmxMap = getTMXMap();
    if (!tmxMap) return false;

    auto backLayer = tmxMap->getLayer("Back");
    if (!backLayer) 
    {
        // 如果没有 Back 层，尝试找 baseLayer
        // 或者直接信任父类的判断
        return true; 
    }

    Vec2 tileCoord = positionToTileCoord(position);
    
    // 边界检查
    Size mapSize = tmxMap->getMapSize();
    if (tileCoord.x < 0 || tileCoord.x >= mapSize.width ||
        tileCoord.y < 0 || tileCoord.y >= mapSize.height)
    {
        return false;
    }

    // 检查 Back 层是否有图块
    int gid = backLayer->getTileGIDAt(tileCoord);
    return gid != 0;
}

