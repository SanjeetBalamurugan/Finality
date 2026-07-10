#pragma once

#include <Finality.h>
#include <string>

namespace FINALITY
{
    class KeyPrintScript : public FINALITY::ScriptableComponent
    {
    public:
        KeyPrintScript() = default;

    private:
        void Start() override
        {
            FI_CLIENT_INFO("KeyPrintScript initialized! Press any key to see its name.");
        }

        void Update(float dt) override
        {
            for (unsigned int i = 32; i < 349; ++i)
            {
                if (Input::IsKeyDown(i))
                {
                    FI_CLIENT_INFO("Key Pressed: {}", GetKeyName(i));
                }
            }
        }

        void OnDestroy() override
        {
            FI_CLIENT_INFO("KeyPrintScript destroyed.");
        }

        std::string GetKeyName(KeyCode key)
        {
            if (key >= Key::A && key <= Key::Z)
            {
                return std::string(1, static_cast<char>(key));
            }
            if (key >= Key::D0 && key <= Key::D9)
            {
                return std::string(1, static_cast<char>(key));
            }

            switch (key)
            {
            case Key::Space: return "SPACE";
            case Key::Apostrophe: return "APOSTROPHE";
            case Key::Comma: return "COMMA";
            case Key::Minus: return "MINUS";
            case Key::Period: return "PERIOD";
            case Key::Slash: return "SLASH";
            case Key::Semicolon: return "SEMICOLON";
            case Key::Equal: return "EQUAL";
            case Key::LeftBracket: return "LEFT BRACKET";
            case Key::Backslash: return "BACKSLASH";
            case Key::RightBracket: return "RIGHT BRACKET";
            case Key::GraveAccent: return "GRAVE ACCENT";
            case Key::World1: return "WORLD 1";
            case Key::World2: return "WORLD 2";
            case Key::Escape: return "ESCAPE";
            case Key::Enter: return "ENTER";
            case Key::Tab: return "TAB";
            case Key::Backspace: return "BACKSPACE";
            case Key::Insert: return "INSERT";
            case Key::Delete: return "DELETE";
            case Key::Right: return "RIGHT ARROW";
            case Key::Left: return "LEFT ARROW";
            case Key::Down: return "DOWN ARROW";
            case Key::Up: return "UP ARROW";
            case Key::PageUp: return "PAGE UP";
            case Key::PageDown: return "PAGE DOWN";
            case Key::Home: return "HOME";
            case Key::End: return "END";
            case Key::CapsLock: return "CAPS LOCK";
            case Key::ScrollLock: return "SCROLL LOCK";
            case Key::NumLock: return "NUM LOCK";
            case Key::PrintScreen: return "PRINT SCREEN";
            case Key::Pause: return "PAUSE";
            case Key::F1: return "F1"; case Key::F2: return "F2"; case Key::F3: return "F3";
            case Key::F4: return "F4"; case Key::F5: return "F5"; case Key::F6: return "F6";
            case Key::F7: return "F7"; case Key::F8: return "F8"; case Key::F9: return "F9";
            case Key::F10: return "F10"; case Key::F11: return "F11"; case Key::F12: return "F12";
            case Key::KP0: return "NUMPAD 0"; case Key::KP1: return "NUMPAD 1";
            case Key::KP2: return "NUMPAD 2"; case Key::KP3: return "NUMPAD 3";
            case Key::KP4: return "NUMPAD 4"; case Key::KP5: return "NUMPAD 5";
            case Key::KP6: return "NUMPAD 6"; case Key::KP7: return "NUMPAD 7";
            case Key::KP8: return "NUMPAD 8"; case Key::KP9: return "NUMPAD 9";
            case Key::KPDecimal: return "NUMPAD DECIMAL";
            case Key::KPDivide: return "NUMPAD DIVIDE";
            case Key::KPMultiply: return "NUMPAD MULTIPLY";
            case Key::KPSubtract: return "NUMPAD SUBTRACT";
            case Key::KPAdd: return "NUMPAD ADD";
            case Key::KPEnter: return "NUMPAD ENTER";
            case Key::KPEqual: return "NUMPAD EQUAL";
            case Key::LeftShift: return "LEFT SHIFT";
            case Key::LeftControl: return "LEFT CONTROL";
            case Key::LeftAlt: return "LEFT ALT";
            case Key::LeftSuper: return "LEFT SUPER";
            case Key::RightShift: return "RIGHT SHIFT";
            case Key::RightControl: return "RIGHT CONTROL";
            case Key::RightAlt: return "RIGHT ALT";
            case Key::RightSuper: return "RIGHT SUPER";
            case Key::Menu: return "MENU";
            default: return "UNKNOWN_KEY_" + std::to_string(key);
            }
        }
    };
}
