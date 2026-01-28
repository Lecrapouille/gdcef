//*****************************************************************************
// MIT License
//
// Copyright (c) 2022 Alain Duron <duron.alain@gmail.com>
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//*****************************************************************************

#include "gdbrowser.hpp"
#include "helper_files.hpp"
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/core/math.hpp>

//------------------------------------------------------------------------------
void GDBrowserView::leftClick()
{
    leftMouseDown();
    leftMouseUp();
}

//------------------------------------------------------------------------------
void GDBrowserView::rightClick()
{
    rightMouseDown();
    rightMouseUp();
}

//------------------------------------------------------------------------------
void GDBrowserView::middleClick()
{
    middleMouseDown();
    middleMouseUp();
}

//------------------------------------------------------------------------------
void GDBrowserView::leftMouseDown()
{
    if (!m_browser)
        return;

    // increase click count but max == 3
    // double-click to select a word.
    // triple-click to select a paragraph.
    // more than triple-click keep paragraph selection.
    m_left_click_count = godot::Math::clamp(m_left_click_count + 1, 1, 3);

    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();
    int64_t click_interval_ms =
        duration_cast<milliseconds>(now - m_last_left_down).count();
    m_last_left_down = now;
    if (click_interval_ms > 500)
        m_left_click_count = 1;

    m_mouse_event_modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn =
        CefBrowserHost::MouseButtonType::MBT_LEFT;
    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseClickEvent(
        evt, btn, false, m_left_click_count);

    // Copy selected text
    // FIXME https://github.com/chromiumembedded/cef/issues/3117
    // if ((m_left_click_count > 1) && m_browser->GetMainFrame())
    //{
    //    m_browser->GetMainFrame()->Copy();
    //}
}

//------------------------------------------------------------------------------
void GDBrowserView::rightMouseDown()
{
    if (!m_browser)
        return;

    m_mouse_event_modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn =
        CefBrowserHost::MouseButtonType::MBT_RIGHT;
    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseClickEvent(evt, btn, false, 1);
}

//------------------------------------------------------------------------------
void GDBrowserView::leftMouseUp()
{
    if (!m_browser)
        return;

    // If an internal HTML5 drag is in progress, end it
    if (m_is_dragging)
    {
        endDragging(m_mouse_x, m_mouse_y);
    }

    m_mouse_event_modifiers &= ~EVENTFLAG_LEFT_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn =
        CefBrowserHost::MouseButtonType::MBT_LEFT;
    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseClickEvent(evt, btn, true, 1);
}

//------------------------------------------------------------------------------
void GDBrowserView::rightMouseUp()
{
    if (!m_browser)
        return;

    m_mouse_event_modifiers &= ~EVENTFLAG_RIGHT_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn =
        CefBrowserHost::MouseButtonType::MBT_RIGHT;
    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseClickEvent(evt, btn, true, 1);
}

//------------------------------------------------------------------------------
void GDBrowserView::middleMouseDown()
{
    if (!m_browser)
        return;

    m_mouse_event_modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn =
        CefBrowserHost::MouseButtonType::MBT_MIDDLE;
    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseClickEvent(evt, btn, false, 1);
}

//------------------------------------------------------------------------------
void GDBrowserView::middleMouseUp()
{
    if (!m_browser)
        return;

    m_mouse_event_modifiers &= ~EVENTFLAG_MIDDLE_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn =
        CefBrowserHost::MouseButtonType::MBT_MIDDLE;
    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseClickEvent(evt, btn, true, 1);
}

//------------------------------------------------------------------------------
void GDBrowserView::mouseMove(int x, int y)
{
    if (!m_browser)
        return;

    m_mouse_x = x;
    m_mouse_y = y;

    CefMouseEvent evt;
    evt.x = x;
    evt.y = y;
    evt.modifiers = m_mouse_event_modifiers;

    auto host = m_browser->GetHost();

    // If an internal HTML5 drag is in progress, update the drag target
    if (m_is_dragging)
    {
        host->DragTargetDragOver(evt, m_drag_allowed_ops);
    }

    bool mouse_leave = false; // TODO
    // AD - Adding focus just like what's done in BLUI
    host->SetFocus(true);
    host->SendMouseMoveEvent(evt, mouse_leave);
}

//------------------------------------------------------------------------------
void GDBrowserView::mouseWheelVertical(int wDelta, bool shift, bool ctrl,
                                        bool alt)
{
    if (m_browser == nullptr)
        return;

    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;

    // Build modifiers from current mouse state + keyboard modifiers
    uint32_t modifiers = m_mouse_event_modifiers;
    if (shift)
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (ctrl)
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (alt)
        modifiers |= EVENTFLAG_ALT_DOWN;
    evt.modifiers = modifiers;

    m_browser->GetHost()->SendMouseWheelEvent(evt, 0, wDelta * 10);
}

//------------------------------------------------------------------------------
void GDBrowserView::mouseWheelHorizontal(int wDelta, bool shift, bool ctrl,
                                          bool alt)
{
    if (m_browser == nullptr)
        return;

    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;

    // Build modifiers from current mouse state + keyboard modifiers
    uint32_t modifiers = m_mouse_event_modifiers;
    if (shift)
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (ctrl)
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (alt)
        modifiers |= EVENTFLAG_ALT_DOWN;
    evt.modifiers = modifiers;

    m_browser->GetHost()->SendMouseWheelEvent(evt, wDelta * 10, 0);
}

// =============================================================================
// Keyboard handling
// =============================================================================

//------------------------------------------------------------------------------
// Build keyboard modifiers flags from individual booleans.
//------------------------------------------------------------------------------
static uint32_t getKeyboardModifiers(bool shift, bool alt, bool ctrl)
{
    uint32_t modifiers = 0;
    if (shift)
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (ctrl)
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (alt)
        modifiers |= EVENTFLAG_ALT_DOWN;
    return modifiers;
}

//------------------------------------------------------------------------------
// Key mapping structure: Godot key code -> Windows virtual key code.
// Reference: https://keycode.info/
//------------------------------------------------------------------------------
struct KeyMapping
{
    int godot_key;    // Godot key constant (e.g., godot::KEY_ENTER)
    int windows_key;  // Windows virtual key code (e.g., VK_RETURN = 13)
    bool send_char;   // Whether to send KEYEVENT_CHAR after KEYDOWN
};

// Static key mapping table for special keys
static const KeyMapping KEY_MAPPINGS[] = {
    // Control keys
    {godot::KEY_BACKSPACE, 8, true},    // VK_BACK
    {godot::KEY_TAB, 9, true},          // VK_TAB
    {godot::KEY_ENTER, 13, true},       // VK_RETURN
    {godot::KEY_KP_ENTER, 13, true},    // VK_RETURN (numpad)
    {godot::KEY_ESCAPE, 27, false},     // VK_ESCAPE

    // Navigation keys
    {godot::KEY_LEFT, 37, false},       // VK_LEFT
    {godot::KEY_UP, 38, false},         // VK_UP
    {godot::KEY_RIGHT, 39, false},      // VK_RIGHT
    {godot::KEY_DOWN, 40, false},       // VK_DOWN
    {godot::KEY_PAGEUP, 33, false},     // VK_PRIOR
    {godot::KEY_PAGEDOWN, 34, false},   // VK_NEXT
    {godot::KEY_HOME, 36, false},       // VK_HOME
    {godot::KEY_END, 35, false},        // VK_END
    {godot::KEY_INSERT, 45, false},     // VK_INSERT
    {godot::KEY_DELETE, 46, false},     // VK_DELETE

    // Function keys (F1-F12)
    {godot::KEY_F1, 112, false},        // VK_F1
    {godot::KEY_F2, 113, false},        // VK_F2
    {godot::KEY_F3, 114, false},        // VK_F3
    {godot::KEY_F4, 115, false},        // VK_F4
    {godot::KEY_F5, 116, false},        // VK_F5
    {godot::KEY_F6, 117, false},        // VK_F6
    {godot::KEY_F7, 118, false},        // VK_F7
    {godot::KEY_F8, 119, false},        // VK_F8
    {godot::KEY_F9, 120, false},        // VK_F9
    {godot::KEY_F10, 121, false},       // VK_F10
    {godot::KEY_F11, 122, false},       // VK_F11
    {godot::KEY_F12, 123, false},       // VK_F12
};

static constexpr size_t KEY_MAPPINGS_COUNT =
    sizeof(KEY_MAPPINGS) / sizeof(KEY_MAPPINGS[0]);

//------------------------------------------------------------------------------
// Look up a Godot key in the mapping table.
// Returns nullptr if not found.
//------------------------------------------------------------------------------
static const KeyMapping* findKeyMapping(int godot_key)
{
    for (size_t i = 0; i < KEY_MAPPINGS_COUNT; ++i)
    {
        if (KEY_MAPPINGS[i].godot_key == godot_key)
        {
            return &KEY_MAPPINGS[i];
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
// Send a key event to the browser.
//
// Key handling in CEF:
// - Printable ASCII characters (32-126): Send KEYEVENT_CHAR directly
// - Special keys (arrows, F-keys, etc.): Send KEYEVENT_KEYDOWN, optionally CHAR
// - Unicode characters (> 127): Send as KEYEVENT_CHAR with proper encoding
//
// Note: Full IME support for CJK (Chinese/Japanese/Korean) input requires
// additional CEF IME integration which is not implemented here.
// Current implementation handles pre-composed Unicode characters.
//------------------------------------------------------------------------------
void GDBrowserView::keyPress(int key,
                             bool pressed,
                             bool shift,
                             bool alt,
                             bool ctrl)
{
    if (!m_browser)
        return;

    CefKeyEvent event;
    event.modifiers = getKeyboardModifiers(shift, alt, ctrl);

    // Handle key release
    if (!pressed)
    {
        // Set key info for KEYUP event
        char16_t key16 = static_cast<char16_t>(key);
        const KeyMapping* mapping = findKeyMapping(key);
        if (mapping != nullptr)
        {
            event.windows_key_code = mapping->windows_key;
            event.native_key_code = mapping->windows_key;
            event.character = static_cast<char16_t>(mapping->windows_key);
            event.unmodified_character = event.character;
        }
        else
        {
            event.windows_key_code = key;
            event.native_key_code = key;
            event.character = key16;
            event.unmodified_character = key16;
        }
        event.native_key_code |= int(0xC0000000);
        event.type = KEYEVENT_KEYUP;
        m_browser->GetHost()->SendKeyEvent(event);
        return;
    }

    // Handle key press
    char16_t key16 = static_cast<char16_t>(key);

    // Check if it's a mapped special key
    const KeyMapping* mapping = findKeyMapping(key);
    if (mapping != nullptr)
    {
        event.windows_key_code = mapping->windows_key;
        event.native_key_code = mapping->windows_key;
        event.character = static_cast<char16_t>(mapping->windows_key);
        event.unmodified_character = event.character;

        // Send KEYDOWN
        event.type = KEYEVENT_KEYDOWN;
        m_browser->GetHost()->SendKeyEvent(event);

        // Send CHAR if needed (for keys like Enter, Backspace)
        if (mapping->send_char)
        {
            event.type = KEYEVENT_CHAR;
            m_browser->GetHost()->SendKeyEvent(event);
        }
        return;
    }

    // Printable ASCII characters (space to tilde)
    if (key >= 32 && key <= 126)
    {
        event.windows_key_code = key;
        event.native_key_code = key;
        event.character = key16;
        event.unmodified_character = key16;

        // Send KEYDOWN first (for games that listen to keydown)
        event.type = KEYEVENT_KEYDOWN;
        m_browser->GetHost()->SendKeyEvent(event);

        // Then send CHAR (for text input)
        event.type = KEYEVENT_CHAR;
        m_browser->GetHost()->SendKeyEvent(event);
        return;
    }

    // Numpad numbers (Godot KEY_KP_0 to KEY_KP_9 = 320-329)
    if (key >= 320 && key <= 329)
    {
        // Convert to ASCII digit (0-9 = 48-57)
        int digit = key - 320;
        event.windows_key_code = 96 + digit;  // VK_NUMPAD0 = 96
        event.character = static_cast<char16_t>('0' + digit);
        event.native_key_code = event.windows_key_code;
        event.unmodified_character = event.character;

        event.type = KEYEVENT_KEYDOWN;
        m_browser->GetHost()->SendKeyEvent(event);
        event.type = KEYEVENT_CHAR;
        m_browser->GetHost()->SendKeyEvent(event);
        return;
    }

    // Unicode characters (> 127) and other keys
    // This handles pre-composed characters from Godot's input system
    // Note: Complex IME input (Chinese, Japanese, Korean composition)
    // requires additional CEF IME APIs not implemented here
    event.windows_key_code = key;
    event.character = key16;
    event.native_key_code = key;
    event.unmodified_character = key16;

    event.type = KEYEVENT_KEYDOWN;
    m_browser->GetHost()->SendKeyEvent(event);
    event.type = KEYEVENT_CHAR;
    m_browser->GetHost()->SendKeyEvent(event);
}
