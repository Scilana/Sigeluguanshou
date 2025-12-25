#ifndef __SKILL_MANAGER_H__
#define __SKILL_MANAGER_H__

#include "cocos2d.h"
#include <array>
#include <cstdint>
#include <string>

class SkillManager : public cocos2d::Node
{
public:
    enum class SkillType
    {
        Agriculture = 0,
        Mining,
        Fishing,
        Cooking,
        Count
    };

    struct SkillData
    {
        std::string name;
        int level = 0;
        int maxLevel = 5;
        int actionCount = 0;
        int actionsPerLevel = 5;
    };

    static SkillManager* getInstance();
    static void destroyInstance();

    virtual bool init() override;

    void recordAction(SkillType type, int count = 1);
    const SkillData& getSkillData(SkillType type) const;
    int getSkillLevel(SkillType type) const;
    int getActionCount(SkillType type) const;
    float getProgressToNextLevel(SkillType type) const;
    uint64_t getVersion() const { return version_; }

    int getMiningHitReduction() const;
    float getFishingCatchGainMultiplier() const;
    float getFishingCatchLossMultiplier() const;
    float getAgricultureBonusChance() const;
    float getCookingSpeedMultiplier() const;

private:
    SkillManager();

    static SkillManager* instance_;
    std::array<SkillData, static_cast<size_t>(SkillType::Count)> skills_;
    uint64_t version_ = 0;

    void recalcLevel(SkillData& skill);
    static size_t toIndex(SkillType type);
};

#endif // __SKILL_MANAGER_H__
