#ifndef __DIALOGUE_BOX_H__
#define __DIALOGUE_BOX_H__

#include "cocos2d.h"
#include "Npc.h"

class DialogueBox : public cocos2d::Node {
public:
    static DialogueBox* create(Npc* npc);
    virtual bool init(Npc* npc);
    void setNpc(Npc* npc);

    // Choice Callback
    typedef std::function<void(int)> ChoiceCallback; // 0 for left option, 1 for right option

    // Callback for general click (to advance text)
    void setOnClickCallback(const std::function<void()>& cb) { _onClickCallback = cb; }

    void showDialogue(const std::string& text);
    void showDialogue(); // Overload for default/compatibility
    void showChoices(const std::string& option1, const std::string& option2, const ChoiceCallback& callback);
    void showNextLine();
    void hideChoices();
    void closeDialogue();
    
    void onMouseDown(cocos2d::Event* event); // Mouse Handler
    
    // Check if dialogue is currently visible
    bool isVisible() const { return _isVisible; }
    bool isWaitingForChoice() const { return _waitingForChoice; }

private:
    Npc* _npc;
    cocos2d::Label* _dialogueLabel;
    cocos2d::Sprite* _background;
    cocos2d::Sprite* _portrait;
    cocos2d::EventListenerMouse* _mouseListener;
    bool _isVisible;
    bool _waitingForChoice;

    // Choice UI
    cocos2d::Node* _choiceNode;
    cocos2d::Label* _option1Label;
    cocos2d::Label* _option2Label;
    ChoiceCallback _choiceCallback;
    std::function<void()> _onClickCallback;

    std::vector<std::string> _lines;
    int _currentLineIndex;

};

#endif // __DIALOGUE_BOX_H__
