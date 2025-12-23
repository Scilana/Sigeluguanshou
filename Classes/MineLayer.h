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
};

#endif // __MINE_LAYER_H__
