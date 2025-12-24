#ifndef __ENERGY_BAR_H__
#define __ENERGY_BAR_H__

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "cocos2d.h"
#undef log
#include "Player.h"

/**
 * @brief 能量条 UI 组件
 */
class EnergyBar : public cocos2d::Node
{
public:
    static EnergyBar* create(Player* player);
    virtual bool init(Player* player);
    virtual void update(float delta) override;

private:
    Player* player_;
    cocos2d::DrawNode* barNode_;
    cocos2d::Label* energyLabel_;
    cocos2d::Sprite* background_;
    
    float barWidth_;
    float barHeight_;

    void updateBar();
};

#endif // __ENERGY_BAR_H__
