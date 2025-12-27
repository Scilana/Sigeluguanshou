#ifndef __BLACKSMITH_UI_H__
#define __BLACKSMITH_UI_H__

#include "cocos2d.h"
#include "InventoryManager.h"

class BlacksmithUI : public cocos2d::Layer
{
public:
    static BlacksmithUI* create();
    virtual bool init();

    /**
     * @brief 显示 UI
     */
    void show();
    void close();

private:
    enum class Mode { Repair, Shop };
    Mode currentMode_ = Mode::Repair;

    void initPanel();
    void refreshList();
    void switchMode(Mode mode);
    void onRepairClicked(int slotIndex);
    void onBuyClicked(ItemType type, int price);

    cocos2d::LayerColor* background_;
    cocos2d::Sprite* panel_;
    cocos2d::Node* listNode_;
};

#endif // __BLACKSMITH_UI_H__
