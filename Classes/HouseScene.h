#ifndef __HOUSE_SCENE_H__
#define __HOUSE_SCENE_H__

#include "cocos2d.h"
#include "Player.h"

class HouseScene : public cocos2d::Scene
{
public:
    static HouseScene* createScene();
    virtual bool init() override;
    virtual void update(float delta) override;

    CREATE_FUNC(HouseScene);

private:
    void initBackground();
    void initPlayer();
    void initControls();
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    Player* player_{ nullptr };
    cocos2d::Sprite* background_{ nullptr };
};

#endif // __HOUSE_SCENE_H__
