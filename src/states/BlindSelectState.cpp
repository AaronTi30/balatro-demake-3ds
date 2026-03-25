#include "BlindSelectState.h"

#include "GameplayState.h"
#include "../core/Application.h"
#include "../core/StateMachine.h"
#include "../core/TextRenderer.h"

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#endif

#include <string>
#include <utility>

BlindSelectState::BlindSelectState(StateMachine* machine, std::shared_ptr<RunState> runState)
    : m_runState(std::move(runState)) {
    m_stateMachine = machine;
}

void BlindSelectState::enter() {
    m_cursorIndex = 0;
    m_inputDelay = 0.3f;
}

void BlindSelectState::exit() {}

bool BlindSelectState::canSkip() const {
    return m_runState->nextBlindStage() != BlindStage::Boss;
}

void BlindSelectState::confirmSelection() {
    if (m_cursorIndex == 1 && canSkip()) {
        m_runState->awardBlindSkip();
        m_runState->advanceBlind();
        m_stateMachine->changeState(
            std::make_shared<BlindSelectState>(m_stateMachine, m_runState));
    } else {
        m_runState->advanceBlind();
        m_stateMachine->changeState(
            std::make_shared<GameplayState>(m_stateMachine, m_runState));
    }
}

void BlindSelectState::update(float dt) {
    if (m_inputDelay > 0.0f) {
        m_inputDelay -= dt;
    }
}

void BlindSelectState::handleInput() {}

void BlindSelectState::renderTopScreen(Application* /*app*/) {}

void BlindSelectState::renderBottomScreen(Application* /*app*/) {}
