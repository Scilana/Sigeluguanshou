#ifndef __NPC_H__
#define __NPC_H__

#include "cocos2d.h"
#include <string>
#include <vector>

class GameScene; // Forward declaration

class Npc : public cocos2d::Sprite {
public:
    enum class NpcType {
        Villager,
        Merchant,
        Blacksmith
    };

    static Npc* create(const std::string& name, const std::string& spriteFile, NpcType type = NpcType::Villager);
    virtual bool init(const std::string& name, const std::string& spriteFile, NpcType type);

    std::string getNpcName() const { return name_; }
    std::string getDialogue() const;
    const std::vector<std::string>& getDialogues() const { return dialogues_; }
    std::string getPortraitFile() const;

    void interact();

    bool isMerchant() const { return type_ == NpcType::Merchant; }

private:
    std::string name_;
    std::vector<std::string> dialogues_;
    NpcType type_;
};

#endif // __NPC_H__
