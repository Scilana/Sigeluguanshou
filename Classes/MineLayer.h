#ifndef __MINE_LAYER_H__
#define __MINE_LAYER_H__

#include "cocos2d.h"
#include "MapLayer.h"
#include <string>

/**
 * @brief 矿洞地图层（继承自 MapLayer）
 *
 * 职责：
 * - 加载和管理 TMX 矿洞地图
 * - 提供矿洞特有的功能（矿物层、楼梯层）
 * - 继承基础的碰撞检测和坐标转换
 */
class MineLayer : public MapLayer
{
public:
    /**
     * @brief 创建矿洞地图层
     * @param tmxFile TMX 地图文件路径
     */
    static MineLayer* create(const std::string& tmxFile);

    /**
     * @brief 初始化
     * @param tmxFile TMX 地图文件路径
     */
    bool init(const std::string& tmxFile);

    /**
     * @brief 检查是否是矿物
     * @param tileCoord 瓦片坐标
     */
    bool isMineralAt(const cocos2d::Vec2& tileCoord) const;

    /**
     * @brief 检查是否是楼梯
     * @param tileCoord 瓦片坐标
     */
    bool isStairsAt(const cocos2d::Vec2& tileCoord) const;

    /**
     * @brief 清除矿物
     * @param tileCoord 瓦片坐标
     */
    void clearMineralAt(const cocos2d::Vec2& tileCoord);

    /**
     * @brief 获取矿物 GID
     * @param tileCoord 瓦片坐标
     */
    int getMineralGID(const cocos2d::Vec2& tileCoord) const;

private:
    cocos2d::TMXLayer* mineralLayer_;      // 矿物层
    cocos2d::TMXLayer* stairsLayer_;       // 楼梯层

    /**
     * @brief 初始化矿洞特有图层
     */
    void initMineLayers();
};

#endif // __MINE_LAYER_H__
