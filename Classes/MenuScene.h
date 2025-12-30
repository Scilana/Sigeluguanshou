#ifndef __MENU_SCENE_H__
#define __MENU_SCENE_H__

#include "cocos2d.h"

class MenuScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init();

    // ťص
    void startGameCallback(cocos2d::Ref* sender);
    void continueGameCallback(cocos2d::Ref* sender);
    void coopCallback(cocos2d::Ref* sender);
    void exitGameCallback(cocos2d::Ref* sender);

    CREATE_FUNC(MenuScene);

private:
    // 
    void createBackground();
    void createLogo();
    void createMenuButtons();
    void createDecorations();
    void addAnimations();

    // 
    bool checkImageExists(const std::string& path);
    cocos2d::MenuItemImage* createImageButton(
        const std::string& normalImage,
        const std::string& selectedImage,
        const cocos2d::ccMenuCallback& callback
    );
};

#endif 
