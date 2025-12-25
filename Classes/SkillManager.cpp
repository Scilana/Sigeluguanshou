#include "SkillManager.h"
#include <algorithm>

USING_NS_CC;

SkillManager* SkillManager::instance_ = nullptr;

SkillManager::SkillManager()
{
}

SkillManager* SkillManager::getInstance()
{
    if (!instance_)
    {
        instance_ = new (std::nothrow) SkillManager();
        if (instance_ && instance_->init())
        {
            instance_->autorelease();
            instance_->retain();
        }
        else
        {
            CC_SAFE_DELETE(instance_);
        }
    }
    return instance_;
}

void SkillManager::destroyInstance()
{
    if (instance_)
    {
        instance_->release();
        instance_ = nullptr;
    }
}

bool SkillManager::init()
{
    if (!Node::init())
        return false;

    skills_[toIndex(SkillType::Agriculture)] = {"Agriculture", 0, 5, 0, 5};
    skills_[toIndex(SkillType::Mining)] = {"Mining", 0, 5, 0, 5};
    skills_[toIndex(SkillType::Fishing)] = {"Fishing", 0, 5, 0, 5};
    skills_[toIndex(SkillType::Cooking)] = {"Cooking", 0, 5, 0, 5};
    version_ = 1;
    return true;
}

void SkillManager::recordAction(SkillType type, int count)
{
    if (count <= 0)
        return;

    auto& skill = skills_[toIndex(type)];
    int prevLevel = skill.level;
    skill.actionCount += count;
    recalcLevel(skill);
    if (skill.level != prevLevel)
    {
        CCLOG("Skill %s leveled up: %d", skill.name.c_str(), skill.level);
        if (levelUpCallback_) {
            levelUpCallback_(type, skill.level);
        }
    }
    version_++;
}

void SkillManager::setSkillData(SkillType type, int level, int actionCount)
{
    auto& skill = skills_[toIndex(type)];
    skill.level = level;
    skill.actionCount = actionCount;
    // 确保等级不超过限制（虽然加载的数据理论上是正确的，但防一防）
    skill.level = std::min(skill.maxLevel, skill.level);
    version_++;
}

const SkillManager::SkillData& SkillManager::getSkillData(SkillType type) const
{
    return skills_[toIndex(type)];
}

int SkillManager::getSkillLevel(SkillType type) const
{
    return skills_[toIndex(type)].level;
}

int SkillManager::getActionCount(SkillType type) const
{
    return skills_[toIndex(type)].actionCount;
}

float SkillManager::getProgressToNextLevel(SkillType type) const
{
    const auto& skill = skills_[toIndex(type)];
    if (skill.level >= skill.maxLevel)
        return 1.0f;
    int remainder = skill.actionCount % skill.actionsPerLevel;
    return static_cast<float>(remainder) / static_cast<float>(skill.actionsPerLevel);
}

int SkillManager::getMiningHitReduction() const
{
    return getSkillLevel(SkillType::Mining);
}

float SkillManager::getFishingCatchGainMultiplier() const
{
    return 1.0f + 0.1f * getSkillLevel(SkillType::Fishing);
}

float SkillManager::getFishingCatchLossMultiplier() const
{
    float mult = 1.0f - 0.05f * getSkillLevel(SkillType::Fishing);
    return std::max(0.5f, mult);
}

float SkillManager::getAgricultureBonusChance() const
{
    return 0.05f * getSkillLevel(SkillType::Agriculture);
}

float SkillManager::getCookingSpeedMultiplier() const
{
    return 1.0f + 0.05f * getSkillLevel(SkillType::Cooking);
}

void SkillManager::recalcLevel(SkillData& skill)
{
    int computed = skill.actionCount / skill.actionsPerLevel;
    skill.level = std::min(skill.maxLevel, computed);
}

size_t SkillManager::toIndex(SkillType type)
{
    return static_cast<size_t>(type);
}
