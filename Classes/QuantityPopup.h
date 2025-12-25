#ifndef __QUANTITY_POPUP_H__
#define __QUANTITY_POPUP_H__

#include "cocos2d.h"
#include <functional>

/**
 * @brief 数量输入弹窗
 */
class QuantityPopup : public cocos2d::Layer
{
public:
    static QuantityPopup* create(int maxVal, const std::function<void(int)>& callback);
    
    bool init(int maxVal, const std::function<void(int)>& callback);
    
private:
    int maxVal_;
    std::function<void(int)> callback_;
    std::string inputText_;
    
    cocos2d::Label* inputLabel_;
    cocos2d::Label* titleLabel_;
    
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void submit();
    void close();
    
    void createUI();
};

#endif // __QUANTITY_POPUP_H__
