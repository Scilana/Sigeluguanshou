#ifndef __FISHING_LAYER_H__
#define __FISHING_LAYER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

class FishingLayer : public Layer
{
public:
    static FishingLayer* create();
    virtual bool init();
    void update(float delta);

    // 设置回调
    void setFinishCallback(std::function<void(bool)> callback);

private:
    void initUI();
    void initInput();
    void updateBarPhysics(float delta);
    void updateFishMovement(float delta);
    void updateProgress(float delta);
    void finishFishing(bool success);

private:
    Sprite* barBase_;
    Sprite* greenBar_;
    Sprite* fishSprite_;
    ui::LoadingBar* progressBar_;

    // Parameters
    float barHeight_;
    float greenBarHeight_;
    
    // Physics
    float barPosition_; // 0.0 - 1.0 (relative to barBottom)
    float barSpeed_;
    float gravity_;
    float thrust_;
    float bounce_;
    
    // Fish
    float fishPosition_; // 0.0 - 1.0
    float fishSpeed_;
    float fishTargetPos_;
    float moveTimer_;

    // Game State
    float catchProgress_; // 0.0 - 1.0
    bool isHolding_;
    bool isGameOver_;

    std::function<void(bool)> finishCallback_;
};

#endif // __FISHING_LAYER_H__
