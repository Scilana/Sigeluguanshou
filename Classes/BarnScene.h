#ifndef __BARN_SCENE_H__
#define __BARN_SCENE_H__

#include "cocos2d.h"
#include <vector>
#include "InventoryManager.h"

class MapLayer;
class Player;
class InventoryUI;

class BarnScene : public cocos2d::Scene
{
public:
    static BarnScene* createScene();
    virtual bool init() override;
    virtual void update(float delta) override;

private:
    MapLayer* mapLayer_{ nullptr };
    Player* player_{ nullptr };
    InventoryManager* inventory_{ nullptr };
    InventoryUI* inventoryUI_{ nullptr };

    cocos2d::Layer* uiLayer_{ nullptr };
    cocos2d::Label* timeLabel_{ nullptr };
    cocos2d::Label* moneyLabel_{ nullptr };
    cocos2d::Label* actionLabel_{ nullptr };
    cocos2d::LayerColor* toolbarUI_{ nullptr };
    std::vector<cocos2d::Sprite*> toolbarSlots_;
    std::vector<cocos2d::Sprite*> toolbarIcons_;
    std::vector<cocos2d::Label*> toolbarCounts_;
    std::vector<int> toolbarCountCache_;
    int toolbarSelectedCache_ = -1;

    std::vector<ItemType> toolbarItems_;
    int selectedItemIndex_ = 0;

    cocos2d::Vec2 exitTile_ = cocos2d::Vec2::ZERO;
    float exitRadius_ = 40.0f;

    void initMap();
    void initPlayer();
    void initCamera();
    void initUI();
    void initControls();
    void initToolbar();
    void initToolbarUI();
    void refreshToolbarUI();
    void selectItemByIndex(int idx);

    void updateCamera();
    void updateUI();
    void toggleInventory();
    void onInventoryClosed();
    void showActionMessage(const std::string& text, const cocos2d::Color3B& color);
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    bool isPlayerAtExit() const;
    void exitBarn();
};

#endif // __BARN_SCENE_H__
