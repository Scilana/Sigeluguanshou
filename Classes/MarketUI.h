#ifndef __MARKET_UI_H__
#define __MARKET_UI_H__

#include "cocos2d.h"
#include <functional>
#include <vector>

class FarmManager;
class InventoryManager;
class MarketState;

class MarketUI : public cocos2d::Layer
{
public:
    static MarketUI* create(InventoryManager* inventory, MarketState* marketState, FarmManager* farmManager);
    bool init(InventoryManager* inventory, MarketState* marketState, FarmManager* farmManager);

    void setCloseCallback(const std::function<void()>& callback);
    void refresh();
    void show();
    void close();
    virtual void update(float delta) override;

private:
    enum class FocusList { Buy, Sell };

    InventoryManager* inventory_ = nullptr;
    MarketState* marketState_ = nullptr;
    FarmManager* farmManager_ = nullptr;

    FocusList focus_ = FocusList::Buy;
    int selectedBuy_ = 0;
    int selectedSell_ = 0;
    int lastDay_ = -1;

    cocos2d::LayerColor* background_ = nullptr;
    cocos2d::LayerColor* panel_ = nullptr;
    cocos2d::Label* moneyLabel_ = nullptr;
    cocos2d::Label* dayLabel_ = nullptr;
    cocos2d::Label* seasonLabel_ = nullptr;
    cocos2d::Label* weatherLabel_ = nullptr;
    cocos2d::Label* infoLabel_ = nullptr;
    cocos2d::Label* statusLabel_ = nullptr;

    cocos2d::Label* buyTitle_ = nullptr;
    cocos2d::Label* sellTitle_ = nullptr;
    std::vector<cocos2d::Label*> buyLabels_;
    std::vector<cocos2d::Label*> sellLabels_;

    std::function<void()> closeCallback_;

    void initBackground();
    void initPanel();
    void initLists();
    void initControls();
    void updateHighlight();
    void updateInfo();
    void moveSelection(int delta);
    void switchFocus();
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void executeTrade();
};

#endif // __MARKET_UI_H__
