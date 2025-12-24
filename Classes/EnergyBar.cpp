#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "cocos2d.h"
#undef log
#include "EnergyBar.h"

USING_NS_CC;

EnergyBar* EnergyBar::create(Player* player)
{
    EnergyBar* ret = new (std::nothrow) EnergyBar();
    if (ret && ret->init(player))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool EnergyBar::init(Player* player)
{
    if (!Node::init())
        return false;

    player_ = player;
    barWidth_ = 20.0f;   // 纵向能量条
    barHeight_ = 150.0f;

    // 背景（外框）
    auto bg = DrawNode::create();
    bg->drawRect(Vec2(-barWidth_/2 - 2, -2), Vec2(barWidth_/2 + 2, barHeight_ + 2), Color4F(0, 0, 0, 0.5f));
    this->addChild(bg, 0);

    // 能量填充部分
    barNode_ = DrawNode::create();
    this->addChild(barNode_, 1);

    // 标签 (E)
    auto label = Label::createWithSystemFont("E", "Arial", 16);
    label->setPosition(Vec2(0, -15));
    this->addChild(label, 2);

    this->scheduleUpdate();
    updateBar();

    return true;
}

void EnergyBar::update(float delta)
{
    updateBar();
}

void EnergyBar::updateBar()
{
    if (!player_ || !barNode_) return;

    float current = player_->getCurrentEnergy();
    float max = player_->getMaxEnergy();
    float percent = clampf(current / max, 0.0f, 1.0f);

    barNode_->clear();

    // 根据百分比选择颜色
    Color4F color;
    if (percent > 0.5f) {
        color = Color4F(0.2f, 0.8f, 0.2f, 1.0f); // 绿色
    } else if (percent > 0.2f) {
        color = Color4F(0.8f, 0.8f, 0.2f, 1.0f); // 黄色
    } else {
        color = Color4F(0.8f, 0.2f, 0.2f, 1.0f); // 红色
    }

    // 绘制填充
    float currentHeight = barHeight_ * percent;
    if (currentHeight > 0) {
        barNode_->drawSolidRect(Vec2(-barWidth_/2, 0), Vec2(barWidth_/2, currentHeight), color);
    }
    
    // 如果力竭，可以加个特殊效果，比如闪烁或者画个 X
    if (player_->isExhausted())
    {
        // 可以在这里加点视觉提示
    }
}
