#include "StateMachine.h"

StateMachine::StateMachine() : 
    m_isAdding(false), 
    m_isRemoving(false), 
    m_isReplacing(false) 
{
}

void StateMachine::pushState(std::shared_ptr<State> state) {
    m_isAdding = true;
    m_isReplacing = false;
    m_newState = state;
}

void StateMachine::popState() {
    m_isRemoving = true;
}

void StateMachine::changeState(std::shared_ptr<State> state) {
    m_isAdding = true;
    m_isReplacing = true;
    m_newState = state;
}

void StateMachine::processStateChanges() {
    if (m_isRemoving && !m_states.empty()) {
        m_states.back()->exit();
        m_states.pop_back();
        
        // Resume the state that is now active
        if (!m_states.empty()) {
            m_states.back()->enter();
        }
        m_isRemoving = false;
    }

    if (m_isAdding) {
        if (!m_states.empty()) {
            if (m_isReplacing) {
                m_states.back()->exit();
                m_states.pop_back();
            }
        }

        m_states.push_back(m_newState);
        m_states.back()->enter();
        
        m_isAdding = false;
    }
}

void StateMachine::handleInput() {
    if (!m_states.empty()) {
        m_states.back()->handleInput();
    }
}

void StateMachine::update(float dt) {
    if (!m_states.empty()) {
        m_states.back()->update(dt);
    }
}

void StateMachine::renderTopScreen(Application* app) {
    if (!m_states.empty()) {
        m_states.back()->renderTopScreen(app);
    }
}

void StateMachine::renderBottomScreen(Application* app) {
    if (!m_states.empty()) {
        m_states.back()->renderBottomScreen(app);
    }
}
