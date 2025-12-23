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

    CCLOG("MineLayer initialized with: %s", tmxFile.c_str());

    // 新的矿洞地图使用 "Buildings" 层作为碰撞层
    auto tmxMap = getTMXMap();
    if (tmxMap)
    {
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
