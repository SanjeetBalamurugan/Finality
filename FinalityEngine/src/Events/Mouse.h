#pragma once
#include <Core/Core.h>
#include <utility>

namespace FINALITY
{
    using MouseCode = uint8_t;

    namespace MouseButton
    {
        enum : MouseCode {
            Button0 = 0,
            Button1 = 1,
            Button2 = 2,
            Button3 = 3,
            Button4 = 4,
            Button5 = 5,
            Button6 = 6,
            Button7 = 7,
            Left = Button0,
            Right = Button1,
            Middle = Button2
        };
    }

    class FAPI Mouse
    {
    public:
        static std::pair<float, float> GetMousePos();
        static float GetMouseX();
        static float GetMouseY();

        static float GetDeltaX();
        static float GetDeltaY();

        static bool IsButtonDown(MouseCode button);
        static bool IsButtonHeld(MouseCode button);
        static bool IsButtonUp(MouseCode button);

        static void ClearFrameStates();
        static void RecordMousePosition(float x, float y);
        static void RecordButtonPress(MouseCode button);
        static void RecordButtonRelease(MouseCode button);

    private:
        static float s_CurrentX;
        static float s_CurrentY;
        static float s_DeltaX;
        static float s_DeltaY;
        static bool s_CurrentButtonStates[8];
        static bool s_PreviousButtonStates[8];
    };
}
