#pragma once
#ifndef __MAP_LAYER_H__
#define __MAP_LAYER_H__

#include "cocos2d.h"

/**
 * TMX 地图包装层：提供碰撞查询、坐标转换和清理瓦片的接口。
 */
class MapLayer : public cocos2d::Layer
{
public:
    static MapLayer* create(const std::string& tmxFile);
    bool init(const std::string& tmxFile);

    cocos2d::TMXTiledMap* getTMXMap() const { return tmxMap_; }
    cocos2d::Size getMapSize() const;
    cocos2d::Size getMapSizeInTiles() const;
    cocos2d::Size getTileSize() const;

    virtual bool isWalkable(const cocos2d::Vec2& position) const;
    bool hasCollisionAt(const cocos2d::Vec2& tileCoord) const;
    void clearCollisionAt(const cocos2d::Vec2& tileCoord);
    void clearBaseTileAt(const cocos2d::Vec2& tileCoord);
    int getBaseTileGID(const cocos2d::Vec2& tileCoord) const;
    void setBaseTileGID(const cocos2d::Vec2& tileCoord, int gid);

    cocos2d::Vec2 positionToTileCoord(const cocos2d::Vec2& position) const;
    cocos2d::Vec2 tileCoordToPosition(const cocos2d::Vec2& tileCoord) const;

private:
    cocos2d::TMXTiledMap* tmxMap_{ nullptr };
    cocos2d::TMXLayer* baseLayer_{ nullptr };
    cocos2d::TMXLayer* collisionLayer_{ nullptr };
    cocos2d::TMXLayer* waterLayer_{ nullptr };

    bool loadTMXMap(const std::string& tmxFile);
    void initCollisionLayer();
};

#endif // __MAP_LAYER_H__
