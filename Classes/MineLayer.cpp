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
    // MapLayer 默认使用 "collision" 层，但新地图使用 "Buildings" 层
    auto tmxMap = getTMXMap();
    if (tmxMap)
    {
        // 检查是否有 Buildings 层（新地图格式）
        auto buildingsLayer = tmxMap->getLayer("Buildings");
        if (buildingsLayer)
        {
            CCLOG("Found Buildings layer in mine map - will use for collision");
        }

        // 检查其他层
        auto backLayer = tmxMap->getLayer("Back");
        auto frontLayer = tmxMap->getLayer("Front");

        if (backLayer) CCLOG("Found Back layer");
        if (frontLayer) CCLOG("Found Front layer");
    }

    return true;
}
