#include "Input.h"

namespace FINALITY
{
    bool Input::s_CurrentFrameStates[512] = { false };
    bool Input::s_PreviousFrameStates[512] = { false };

    bool Input::IsKeyDown(KeyCode key) {
        if (key >= 512) return false;
        return s_CurrentFrameStates[key] && !s_PreviousFrameStates[key];
    }

    bool Input::IsKeyHeld(KeyCode key) {
        if (key >= 512) return false;
        return s_CurrentFrameStates[key] && s_PreviousFrameStates[key];
    }

    bool Input::IsKeyUp(KeyCode key) {
        if (key >= 512) return false;
        return !s_CurrentFrameStates[key] && s_PreviousFrameStates[key];
    }

    void Input::ClearFrameStates() {
        for (int i = 0; i < 512; i++) {
            s_PreviousFrameStates[i] = s_CurrentFrameStates[i];
        }
    }

    void Input::RecordKeyPress(KeyCode key) {
        if (key < 512) s_CurrentFrameStates[key] = true;
    }

    void Input::RecordKeyRelease(KeyCode key) {
        if (key < 512) s_CurrentFrameStates[key] = false;
    }
}
