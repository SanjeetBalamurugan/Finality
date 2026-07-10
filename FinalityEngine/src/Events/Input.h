#pragma once

#include <Core/Core.h>
#include "KeyCodes.h"

namespace FINALITY
{
    class FAPI Input
    {
    public:
        static bool IsKeyDown(KeyCode key);
        static bool IsKeyHeld(KeyCode key);
        static bool IsKeyUp(KeyCode key);

        static void ClearFrameStates();
        static void RecordKeyPress(KeyCode key);
        static void RecordKeyRelease(KeyCode key);

    private:
        static bool s_CurrentFrameStates[512];
        static bool s_PreviousFrameStates[512];
    };
}
