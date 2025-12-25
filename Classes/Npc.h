#ifndef __NPC_H__
#define __NPC_H__

#include "cocos2d.h"
#include <string>
#include <vector>

class Npc : public cocos2d::Sprite {
public:
    static Npc* create(const std::string& name, const std::string& spriteFile);
    virtual bool init(const std::string& name, const std::string& spriteFile);

    std::string getNpcName() const { return _name; }
    std::string getDialogue() const;
    std::vector<std::string> getDialogues() const { return _dialogues; }
    std::string getPortraitFile() const;

    void interact();

private:
    std::string _name;
    std::vector<std::string> _dialogues;
};

#endif // __NPC_H__
