#ifndef __SKILL_TREE_UI_H__
#define __SKILL_TREE_UI_H__

#include "cocos2d.h"
#include "SkillManager.h"
#include <vector>

class SkillTreeUI : public cocos2d::Layer
{
public:
    static SkillTreeUI* create();
    virtual bool init() override;

    void refresh();
    void show();
    void close();

private:
    struct SkillRow
    {
        SkillManager::SkillType type;
        cocos2d::Label* nameLabel = nullptr;
        cocos2d::Label* levelLabel = nullptr;
        cocos2d::Label* countLabel = nullptr;
        cocos2d::LayerColor* barFill = nullptr;
        float barWidth = 0.0f;
    };

    cocos2d::LayerColor* background_ = nullptr;
    cocos2d::LayerColor* panel_ = nullptr;
    std::vector<SkillRow> rows_;
    uint64_t lastVersion_ = 0;

    void buildLayout();
    void updateRow(SkillRow& row, const SkillManager::SkillData& data);
    void update(float delta) override;
};

#endif // __SKILL_TREE_UI_H__
