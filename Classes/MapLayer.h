#pragma once
#ifndef __MAP_LAYER_H__
#define __MAP_LAYER_H__

#include "cocos2d.h"

/**
 * @brief 地图层类
 *
 * 负责：
 * - 加载和显示TMX地图
 * - 提供地图信息（大小、碰撞等）
 * - 地图相关的查询功能
 */
class MapLayer : public cocos2d::Layer
{
public:
    /**
     * @brief 创建地图层
     * @param tmxFile TMX文件路径
     * @return MapLayer对象
     */
    static MapLayer* create(const std::string& tmxFile);

    /**
     * @brief 初始化
     * @param tmxFile TMX文件路径
     */
    bool init(const std::string& tmxFile);

    /**
     * @brief 获取TMX地图对象
     */
    cocos2d::TMXTiledMap* getTMXMap() const { return tmxMap_; }

    /**
     * @brief 获取地图大小（像素）
     */
    cocos2d::Size getMapSize() const;

    /**
     * @brief 获取地图大小（格子数）
     */
    cocos2d::Size getMapSizeInTiles() const;

    /**
     * @brief 获取瓦片大小
     */
    cocos2d::Size getTileSize() const;

    /**
     * @brief 检查某个位置是否可行走
     * @param position 世界坐标位置
     * @return true = 可行走，false = 不可行走
     */
    bool isWalkable(const cocos2d::Vec2& position) const;

    /**
     * @brief 瓦片坐标是否存在碰撞
     */
    bool hasCollisionAt(const cocos2d::Vec2& tileCoord) const;

    /**
     * @brief 清除碰撞瓦片（用于砍树后移除碰撞）
     */
    void clearCollisionAt(const cocos2d::Vec2& tileCoord);

    /**
     * @brief 将世界坐标转换为瓦片坐标
     * @param position 世界坐标
     * @return 瓦片坐标
     */
    cocos2d::Vec2 positionToTileCoord(const cocos2d::Vec2& position) const;

    /**
     * @brief 将瓦片坐标转换为世界坐标
     * @param tileCoord 瓦片坐标
     * @return 世界坐标
     */
    cocos2d::Vec2 tileCoordToPosition(const cocos2d::Vec2& tileCoord) const;

private:
    // TMX地图对象
    cocos2d::TMXTiledMap* tmxMap_;

    // 碰撞层（用于检测不可行走的区域）
    cocos2d::TMXLayer* collisionLayer_;

    /**
     * @brief 加载TMX地图
     * @param tmxFile TMX文件路径
     */
    bool loadTMXMap(const std::string& tmxFile);

    /**
     * @brief 初始化碰撞层
     */
    void initCollisionLayer();
};

#endif // __MAP_LAYER_H__
