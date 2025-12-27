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
    void setOnClickCallback(const std::function<void()>& cb) { on_click_callback_ = cb; }

    void showDialogue(const std::string& text);
    void showDialogue(); // Overload for default/compatibility
    void showChoices(const std::string& option1, const std::string& option2, const ChoiceCallback& callback);
    void showNextLine();
    void hideChoices();
    void closeDialogue();

    void onMouseDown(cocos2d::Event* event); // Mouse Handler

    // Check if dialogue is currently visible
    bool isVisible() const { return is_visible_; }
    bool isWaitingForChoice() const { return waiting_for_choice_; }

private:
    Npc* npc_;
    cocos2d::Label* dialogue_label_;
    cocos2d::Sprite* background_;
    cocos2d::Sprite* portrait_;
    cocos2d::EventListenerMouse* mouse_listener_;
    bool is_visible_;
    bool waiting_for_choice_;

    // Choice UI
    cocos2d::Node* choice_node_;
    cocos2d::Label* option1_label_;
    cocos2d::Label* option2_label_;
    ChoiceCallback choice_callback_;
    std::function<void()> on_click_callback_;

    std::vector<std::string> lines_;
    int current_line_index_;

};

#endif // __DIALOGUE_BOX_H__
