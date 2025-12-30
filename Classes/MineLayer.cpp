#include "MineLayer.h"

USING_NS_CC;

MineLayer* MineLayer::create(const std::string& tmxFile) {
  MineLayer* ret = new (std::nothrow) MineLayer();
  if (ret && ret->init(tmxFile)) {
    ret->autorelease();
    return ret;
  }
  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool MineLayer::init(const std::string& tmxFile) {
  // 调用父类初始化加载地图
  if (!MapLayer::init(tmxFile)) {
    return false;
  }

  mineralLayer_ = nullptr;
  stairsLayer_ = nullptr;

  // 新的矿洞地图使用建筑层作为碰撞层
  auto tmxMap = getTMXMap();
  if (tmxMap) {
    mineralLayer_ = tmxMap->getLayer("mineral");
    if (!mineralLayer_) mineralLayer_ = tmxMap->getLayer("Mineral");
    if (!mineralLayer_) mineralLayer_ = tmxMap->getLayer("mine1");

    stairsLayer_ = tmxMap->getLayer("stairs");
    if (!stairsLayer_) stairsLayer_ = tmxMap->getLayer("Stairs");

    // 检查图层
    auto backLayer = tmxMap->getLayer("Back");
    auto buildingsLayer = tmxMap->getLayer("Buildings");
    auto frontLayer = tmxMap->getLayer("Front");
  }

  return true;
}

bool MineLayer::isMineralAt(const Vec2& tileCoord) const {
  if (!mineralLayer_) return false;

  Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
  if (tileCoord.x < 0 || tileCoord.x >= mapSize.width || tileCoord.y < 0 ||
      tileCoord.y >= mapSize.height)
    return false;

  return mineralLayer_->getTileGIDAt(tileCoord) != 0;
}

int MineLayer::getMineralGID(const Vec2& tileCoord) const {
  if (!mineralLayer_) return 0;

  Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
  if (tileCoord.x < 0 || tileCoord.x >= mapSize.width || tileCoord.y < 0 ||
      tileCoord.y >= mapSize.height)
    return 0;

  return mineralLayer_->getTileGIDAt(tileCoord);
}

void MineLayer::clearMineralAt(const Vec2& tileCoord) {
  if (!mineralLayer_) return;

  Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
  if (tileCoord.x < 0 || tileCoord.x >= mapSize.width || tileCoord.y < 0 ||
      tileCoord.y >= mapSize.height)
    return;

  mineralLayer_->setTileGID(0, tileCoord);
}

bool MineLayer::isStairsAt(const Vec2& tileCoord) const {
  if (!stairsLayer_) return false;

  Size mapSize = getTMXMap() ? getTMXMap()->getMapSize() : Size::ZERO;
  if (tileCoord.x < 0 || tileCoord.x >= mapSize.width || tileCoord.y < 0 ||
      tileCoord.y >= mapSize.height)
    return false;

  return stairsLayer_->getTileGIDAt(tileCoord) != 0;
}

bool MineLayer::isWalkable(const Vec2& position) const {
  // 1. 检查障碍物（建筑层，通过父类检查）
  if (!MapLayer::isWalkable(position)) return false;

  // 2. 检查地面（背景层）
  // 只有背景层有图块的地方才是地面，没有图块则为虚空或墙壁
  auto tmxMap = getTMXMap();
  if (!tmxMap) return false;

  auto backLayer = tmxMap->getLayer("Back");
  if (!backLayer) {
    // 如果没有背景层，尝试找基础层
    // 或者直接信任父类的判断
    return true;
  }

  Vec2 tileCoord = positionToTileCoord(position);

  // 边界检查
  Size mapSize = tmxMap->getMapSize();
  if (tileCoord.x < 0 || tileCoord.x >= mapSize.width || tileCoord.y < 0 ||
      tileCoord.y >= mapSize.height) {
    return false;
  }

  // 检查背景层是否有图块
  int gid = backLayer->getTileGIDAt(tileCoord);
  return gid != 0;
}
