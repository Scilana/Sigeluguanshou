#ifndef __DIALOGUE_BOX_H__
#define __DIALOGUE_BOX_H__

#include "cocos2d.h"
#include "Npc.h"

class DialogueBox : public cocos2d::Node {
public:
    static DialogueBox* create(Npc* npc);
    virtual bool init(Npc* npc);
    void setNpc(Npc* npc);

    void showDialogue();
    void showNextLine();
    void closeDialogue();
    
    // Check if dialogue is currently visible
    bool isVisible() const { return _isVisible; }

private:
    Npc* _npc;
    cocos2d::Label* _dialogueLabel;
    cocos2d::Sprite* _background;
    cocos2d::Sprite* _portrait;
    cocos2d::EventListenerMouse* _mouseListener;
    bool _isVisible;

    std::vector<std::string> _lines;
    int _currentLineIndex;

    void onMouseDown(cocos2d::Event* event);
};

#endif // __DIALOGUE_BOX_H__
