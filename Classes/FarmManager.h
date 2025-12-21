#ifndef __FARM_MANAGER_H__
#define __FARM_MANAGER_H__

#include "cocos2d.h"
#include <string>
#include <unordered_map>
#include <vector>

class MapLayer;

/**
 * @brief 管理农田状态（耕地、浇水、作物生长）并绘制覆盖层
 *
 * - 使用 TMX 地图尺寸自动匹配瓦片
 * - 简单的时间推进：默认每 5 秒+1 天，浇水的作物会前进生长阶段
 * - 提供耕地、种植、浇水、收获的动作接口
 */
class FarmManager : public cocos2d::Node
{
public:
    struct ActionResult
    {
        bool success;
        std::string message;
    };

    static FarmManager* create(MapLayer* mapLayer);
    bool init(MapLayer* mapLayer);
    virtual void update(float delta) override;

    ActionResult tillTile(const cocos2d::Vec2& tileCoord);
    ActionResult plantSeed(const cocos2d::Vec2& tileCoord, int cropId = 0);
    ActionResult waterTile(const cocos2d::Vec2& tileCoord);
    ActionResult harvestTile(const cocos2d::Vec2& tileCoord);

    int getDayCount() const { return dayCount_; }
    void forceRedraw();

private:
    struct FarmTile
    {
        bool tilled = false;
        bool watered = false;
        bool hasCrop = false;
        int cropId = -1;
        int stage = 0;            // 当前生长阶段索引
        int progressDays = 0;     // 当前阶段已积累的天数
    };

    struct CropDef
    {
        int id;
        std::string name;
        std::vector<int> stageDays; // 每个阶段需要的天数
        int salePrice;
    };

    void initCropDefs();
    void progressDay();
    void redrawOverlay();
    bool isValidTile(const cocos2d::Vec2& tileCoord) const;
    CropDef getCropDef(int cropId) const;
    bool isMature(const FarmTile& tile) const;

    MapLayer* mapLayer_;
    cocos2d::Size mapSizeTiles_;
    cocos2d::Size tileSize_;
    cocos2d::DrawNode* overlay_;

    float dayTimer_;
    float secondsPerDay_;
    int dayCount_;

    std::vector<FarmTile> tiles_;
    std::unordered_map<int, CropDef> crops_;
};

#endif // __FARM_MANAGER_H__
