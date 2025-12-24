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
 * - 继承基础的碰撞检测和坐标转换
 * - 使用 Buildings 层作为碰撞层
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

    bool isMineralAt(const cocos2d::Vec2& tileCoord) const;
    int getMineralGID(const cocos2d::Vec2& tileCoord) const;
    void clearMineralAt(const cocos2d::Vec2& tileCoord);
    bool isStairsAt(const cocos2d::Vec2& tileCoord) const;

    // 重写 isWalkable，检查地面是否存在
    virtual bool isWalkable(const cocos2d::Vec2& position) const override;

private:
    cocos2d::TMXLayer* mineralLayer_{ nullptr };
    cocos2d::TMXLayer* stairsLayer_{ nullptr };
};

#endif // __MINE_LAYER_H__
