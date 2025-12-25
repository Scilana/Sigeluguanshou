#include "SkillTreeUI.h"

USING_NS_CC;

SkillTreeUI* SkillTreeUI::create()
{
    SkillTreeUI* ret = new (std::nothrow) SkillTreeUI();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool SkillTreeUI::init()
{
    if (!Layer::init())
        return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();

    background_ = LayerColor::create(Color4B(0, 0, 0, 180), visibleSize.width, visibleSize.height);
    addChild(background_, 0);

    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* touch, Event* event) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, background_);

    panel_ = LayerColor::create(Color4B(40, 35, 30, 255), 520, 360);
    panel_->setIgnoreAnchorPointForPosition(false);
    panel_->setAnchorPoint(Vec2(0.5f, 0.5f));
    panel_->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
    addChild(panel_, 1);

    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), panel_->getContentSize(),
        Color4F(0.8f, 0.7f, 0.5f, 1.0f));
    border->setLineWidth(3.0f);
    panel_->addChild(border, 0);

    auto title = Label::createWithSystemFont("Skill Tree (E to close)", "Arial", 24);
    title->setPosition(Vec2(panel_->getContentSize().width / 2, panel_->getContentSize().height - 30));
    title->setColor(Color3B(255, 235, 200));
    panel_->addChild(title, 1);

    buildLayout();
    refresh();
    this->scheduleUpdate();
    return true;
}

void SkillTreeUI::buildLayout()
{
    rows_.clear();

    struct SkillInfo
    {
        SkillManager::SkillType type;
    };

    std::vector<SkillInfo> list = {
        {SkillManager::SkillType::Agriculture},
        {SkillManager::SkillType::Mining},
        {SkillManager::SkillType::Fishing},
        {SkillManager::SkillType::Cooking},
    };

    float startY = panel_->getContentSize().height - 80.0f;
    float rowGap = 65.0f;
    float leftX = 40.0f;
    float barX = 180.0f;
    float barWidth = 220.0f;
    float barHeight = 16.0f;

    for (size_t i = 0; i < list.size(); ++i)
    {
        float y = startY - rowGap * static_cast<float>(i);

        SkillRow row;
        row.type = list[i].type;
        row.barWidth = barWidth;

        row.nameLabel = Label::createWithSystemFont("", "Arial", 20);
        row.nameLabel->setAnchorPoint(Vec2(0, 0.5f));
        row.nameLabel->setPosition(Vec2(leftX, y));
        row.nameLabel->setColor(Color3B(230, 220, 200));
        panel_->addChild(row.nameLabel, 1);

        auto barBg = LayerColor::create(Color4B(80, 80, 80, 255), barWidth, barHeight);
        barBg->setIgnoreAnchorPointForPosition(false);
        barBg->setAnchorPoint(Vec2(0, 0.5f));
        barBg->setPosition(Vec2(barX, y));
        panel_->addChild(barBg, 1);

        row.barFill = LayerColor::create(Color4B(120, 200, 120, 255), 0, barHeight);
        row.barFill->setIgnoreAnchorPointForPosition(false);
        row.barFill->setAnchorPoint(Vec2(0, 0.5f));
        row.barFill->setPosition(Vec2(0, barHeight / 2));
        barBg->addChild(row.barFill, 1);

        row.levelLabel = Label::createWithSystemFont("", "Arial", 18);
        row.levelLabel->setAnchorPoint(Vec2(0, 0.5f));
        row.levelLabel->setPosition(Vec2(barX + barWidth + 20.0f, y));
        row.levelLabel->setColor(Color3B(200, 200, 200));
        panel_->addChild(row.levelLabel, 1);

        row.countLabel = Label::createWithSystemFont("", "Arial", 16);
        row.countLabel->setAnchorPoint(Vec2(0, 0.5f));
        row.countLabel->setPosition(Vec2(leftX, y - 22.0f));
        row.countLabel->setColor(Color3B(160, 160, 160));
        panel_->addChild(row.countLabel, 1);

        rows_.push_back(row);
    }
}

void SkillTreeUI::refresh()
{
    auto manager = SkillManager::getInstance();
    for (auto& row : rows_)
    {
        const auto& data = manager->getSkillData(row.type);
        updateRow(row, data);
    }
    lastVersion_ = manager->getVersion();
}

void SkillTreeUI::show()
{
    setVisible(true);
    setOpacity(0);
    runAction(FadeIn::create(0.15f));
}

void SkillTreeUI::close()
{
    removeFromParent();
}

void SkillTreeUI::updateRow(SkillRow& row, const SkillManager::SkillData& data)
{
    if (row.nameLabel)
        row.nameLabel->setString(data.name);

    if (row.levelLabel)
    {
        std::string levelText = StringUtils::format("Lv %d/%d", data.level, data.maxLevel);
        row.levelLabel->setString(levelText);
    }

    if (row.countLabel)
    {
        std::string countText = StringUtils::format("Actions: %d", data.actionCount);
        row.countLabel->setString(countText);
    }

    float progress = SkillManager::getInstance()->getProgressToNextLevel(row.type);
    float fillWidth = row.barWidth * progress;
    if (row.barFill)
    {
        row.barFill->setContentSize(Size(fillWidth, row.barFill->getContentSize().height));
    }
}

void SkillTreeUI::update(float delta)
{
    auto manager = SkillManager::getInstance();
    if (manager->getVersion() != lastVersion_)
    {
        refresh();
    }
}
