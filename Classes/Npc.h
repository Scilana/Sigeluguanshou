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
        Merchant
    };

    static Npc* create(const std::string& name, const std::string& spriteFile, NpcType type = NpcType::Villager);
    virtual bool init(const std::string& name, const std::string& spriteFile, NpcType type);

    std::string getNpcName() const { return _name; }
    std::string getDialogue() const;
    const std::vector<std::string>& getDialogues() const { return _dialogues; }
    std::string getPortraitFile() const;

    void interact(); 
    
    bool isMerchant() const { return _type == NpcType::Merchant; }

private:
    std::string _name;
    std::vector<std::string> _dialogues;
    NpcType _type;
};

#endif // __NPC_H__
