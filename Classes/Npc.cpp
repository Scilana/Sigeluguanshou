#include "Npc.h"

USING_NS_CC;

Npc* Npc::create(const std::string& name, const std::string& spriteFile, NpcType type) {
    Npc* npc = new (std::nothrow) Npc();
    if (npc && npc->init(name, spriteFile, type)) {
        npc->autorelease();
        return npc;
    }
    CC_SAFE_DELETE(npc);
    return nullptr;
}

bool Npc::init(const std::string& name, const std::string& spriteFile, NpcType type) {
    if (!Sprite::initWithFile(spriteFile)) {
        return false;
    }
    _name = name;
    _type = type;
    
    // Basic Dialogues
    _dialogues.push_back("Hello there, traveler!");
    _dialogues.push_back("Nice weather today.");
    _dialogues.push_back("I love this island.");

    return true;
}

std::string Npc::getDialogue() const {
    if (_dialogues.empty()) return "...";
    // Random dialogue
    int idx = cocos2d::random(0, (int)_dialogues.size() - 1);
    return _dialogues[idx];
}

std::string Npc::getPortraitFile() const {
    // Determine portrait file based on name
    // Assuming resources are in Resources/npcImages/
    // e.g., "Wizard" -> "Wizard YuuuTalk.png"
    if (_name == "Wizard") return "npcImages/Wizard YuuuTalk.png";
    if (_name == "Cleaner") return "npcImages/Cleaner LeviTalk.png";
    return "npcImages/Wizard YuuuTalk.png"; // Default
}

void Npc::interact() {
    CCLOG("Interacting with NPC: %s", _name.c_str());
}
