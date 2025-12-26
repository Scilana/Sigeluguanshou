#ifndef __BEACH_SCENE_H__
#define __BEACH_SCENE_H__

#include "cocos2d.h"
#include <vector>
#include "InventoryManager.h"

class MapLayer;
class Player;
class InventoryUI;

class BeachScene : public cocos2d::Scene
{
public:
    static BeachScene* createScene(InventoryManager* inventory, int dayCount = 1, float accumulatedSeconds = 0.0f);
    virtual bool init(InventoryManager* inventory, int dayCount, float accumulatedSeconds);
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

    int dayCount_ = 1;
    float accumulatedSeconds_ = 0.0f;
    float secondsPerDay_ = 120.0f;
    float exitCooldown_ = 0.0f;
    bool transitioning_ = false;

    void initMap();
    void initPlayer();
    void initCamera();
    void initUI();
    void initControls();
    void initToolbar();
    void initToolbarUI();
    void refreshToolbarUI();
    void selectItemByIndex(int idx);

    void onMouseDown(cocos2d::Event* event);
    void onMouseUp(cocos2d::Event* event);
    void startFishing();
    void updateFishingState(float delta);

    void updateCamera();
    void updateUI();
    void toggleInventory();
    void onInventoryClosed();
    void backToFarm();
    bool isPlayerAtFarmExit() const;
    void showActionMessage(const std::string& text, const cocos2d::Color3B& color);
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

    enum class FishingState { NONE, CHARGING, WAITING, BITING, REELING };
    FishingState fishingState_ = FishingState::NONE;
    float chargePower_ = 0.0f;
    float fishingTimer_ = 0.0f;
    float waitTimer_ = 0.0f;
    float biteTimer_ = 0.0f;
    bool isFishing_ = false;

    cocos2d::Sprite* chargeBarBg_ = nullptr;
    cocos2d::Sprite* chargeBarFg_ = nullptr;
    cocos2d::Sprite* exclamationMark_ = nullptr;
};

#endif // __BEACH_SCENE_H__
