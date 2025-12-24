#ifndef __HOUSE_SCENE_H__
#define __HOUSE_SCENE_H__

#include "cocos2d.h"
class FarmManager;
class Player;

class HouseScene : public cocos2d::Scene
{
public:
    static HouseScene* createScene();
    virtual bool init() override;
    virtual void update(float delta) override;

    CREATE_FUNC(HouseScene);

    void setFarmManager(FarmManager* farmManager) { farmManager_ = farmManager; }

private:
    void initBackground();
    void initPlayer();
    void initControls();
    void initUI();
    void updateUI();
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    Player* player_{ nullptr };
    cocos2d::Sprite* background_{ nullptr };
    FarmManager* farmManager_{ nullptr };
    cocos2d::Label* timeLabel_{ nullptr };
    bool isSleeping_{ false };
};

#endif // __HOUSE_SCENE_H__
