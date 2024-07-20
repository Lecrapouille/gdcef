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
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
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
    int64_t click_interval_ms = duration_cast<milliseconds>(now - m_last_left_down).count();
    m_last_left_down = now;
    if (click_interval_ms > 500)
        m_left_click_count = 1;

    m_mouse_event_modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn = CefBrowserHost::MouseButtonType::MBT_LEFT;
    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseClickEvent(evt, btn, false, m_left_click_count);

    // Copy selected text
    // FIXME https://github.com/chromiumembedded/cef/issues/3117
    //if ((m_left_click_count > 1) && m_browser->GetMainFrame())
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

    CefBrowserHost::MouseButtonType btn = CefBrowserHost::MouseButtonType::MBT_RIGHT;
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

    m_mouse_event_modifiers &= ~EVENTFLAG_LEFT_MOUSE_BUTTON;

    CefBrowserHost::MouseButtonType btn = CefBrowserHost::MouseButtonType::MBT_LEFT;
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

    CefBrowserHost::MouseButtonType btn = CefBrowserHost::MouseButtonType::MBT_RIGHT;
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

    CefBrowserHost::MouseButtonType btn = CefBrowserHost::MouseButtonType::MBT_MIDDLE;
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

    CefBrowserHost::MouseButtonType btn = CefBrowserHost::MouseButtonType::MBT_MIDDLE;
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
        return ;

    m_mouse_x = x;
    m_mouse_y = y;

    CefMouseEvent evt;
    evt.x = x;
    evt.y = y;
    evt.modifiers = m_mouse_event_modifiers;

    bool mouse_leave = false; // TODO
    // AD - Adding focus just like what's done in BLUI
    auto host = m_browser->GetHost();
    host->SetFocus(true);
    host->SendMouseMoveEvent(evt, mouse_leave);
}

//------------------------------------------------------------------------------
void GDBrowserView::mouseWheelVertical(const int wDelta)
{
    if (m_browser == nullptr)
        return ;

    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseWheelEvent(evt, 0, wDelta * 10);
}

//------------------------------------------------------------------------------
void GDBrowserView::mouseWheelHorizontal(const int wDelta)
{
    if (m_browser == nullptr)
        return ;

    CefMouseEvent evt;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;
    evt.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->SendMouseWheelEvent(evt, wDelta * 10, 0);
}

//------------------------------------------------------------------------------
static uint32_t getKeyboardModifiers(bool shift, bool alt, bool ctrl)
{
    uint32_t modifiers = 0;

    if (shift == true)
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (ctrl == true)
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (alt == true)
        modifiers |= EVENTFLAG_ALT_DOWN;

    return modifiers;
}

//------------------------------------------------------------------------------
void GDBrowserView::keyPress(int key, bool pressed, bool shift, bool alt, bool ctrl)
{
    if (!m_browser)
        return;

    CefKeyEvent event;
    char16_t key16b = char16_t(key);
    if (pressed == true)
    {
        // set the event modifier if they are activated
        event.modifiers = getKeyboardModifiers(shift, alt, ctrl);

        if ((key >= 32) && (key <= 126)) // ASCII
        {
            BROWSER_DEBUG_VAL("ASCII CODE");
            event.windows_key_code = key;
            event.character = key16b;
            event.unmodified_character = key16b;
            event.type = KEYEVENT_CHAR;
            m_browser->GetHost()->SendKeyEvent(event);
        }
        else if (key == godot::KEY_SPACE ||
                 key == godot::KEY_TAB)
        {
            BROWSER_DEBUG_VAL("KEY_SPACE / KEY_TAB");
            event.windows_key_code = key;
            event.character = key16b;
            event.native_key_code = key;
            event.type = KEYEVENT_RAWKEYDOWN;
            m_browser->GetHost()->SendKeyEvent(event);
            event.type = KEYEVENT_CHAR;
            m_browser->GetHost()->SendKeyEvent(event);
        }
        else if (key == godot::KEY_BACKSPACE ||
                 key == godot::KEY_ENTER ||
                 key == godot::KEY_KP_ENTER )
        {
            if (key == godot::KEY_BACKSPACE) {
                BROWSER_DEBUG_VAL("KEY_BACKSPACE");
                event.windows_key_code = 8;
                event.character = 8;
                event.unmodified_character = 8;
            }
            else if (key == godot::KEY_ENTER) {
                BROWSER_DEBUG_VAL("KEY_ENTER");
                event.windows_key_code = 13;
                event.character = 13;
                event.unmodified_character = 13;
            }
            else if (key == godot::KEY_KP_ENTER) {
                BROWSER_DEBUG_VAL("KEY_KP_ENTER");
                event.windows_key_code = 13;
                event.character = 13;
                event.unmodified_character = 13;
            }

            event.character = char16_t(event.windows_key_code);
            event.native_key_code = event.windows_key_code;
            event.type = KEYEVENT_KEYDOWN;
            m_browser->GetHost()->SendKeyEvent(event);
            event.type = KEYEVENT_CHAR;
            m_browser->GetHost()->SendKeyEvent(event);
        }
        else if (key >= 320 && key <= 329) // NUMBERS & NUMPAD
        {
            BROWSER_DEBUG_VAL("NUMBERS and NUMPAD");
            event.windows_key_code = key;
            event.character = key16b;
            event.native_key_code = key;

            event.type = KEYEVENT_KEYDOWN;
            m_browser->GetHost()->SendKeyEvent(event);
            event.type = KEYEVENT_CHAR;
            m_browser->GetHost()->SendKeyEvent(event);
        }
        else if (key == godot::KEY_RIGHT ||
                 key == godot::KEY_LEFT ||
                 key == godot::KEY_UP ||
                 key == godot::KEY_DOWN ||
                 key == godot::KEY_PAGEUP ||
                 key == godot::KEY_PAGEDOWN ||
                 key == godot::KEY_HOME ||
                 key == godot::KEY_END ||
                 key == godot::KEY_INSERT ||
                 key == godot::KEY_DELETE) // ARROWS
        {
            // https://keycode.info/

            if (key == godot::KEY_RIGHT) {
                BROWSER_DEBUG_VAL("KEY_RIGHT");
                event.windows_key_code = 39;
            }
            else if (key == godot::KEY_LEFT) {
                BROWSER_DEBUG_VAL("KEY_LEFT");
                event.windows_key_code = 37;
            }
            else if (key == godot::KEY_UP) {
                BROWSER_DEBUG_VAL("KEY_UP");
                event.windows_key_code = 38;
            }
            else if (key == godot::KEY_DOWN) {
                BROWSER_DEBUG_VAL("KEY_DOWN");
                event.windows_key_code = 40;
            }
            else if (key == godot::KEY_PAGEUP) {
                BROWSER_DEBUG_VAL("KEY_PAGEUP");
                event.windows_key_code = 33;
            }
            else if (key == godot::KEY_PAGEDOWN) {
                BROWSER_DEBUG_VAL("KEY_PAGEDOWN");
                event.windows_key_code = 34;
            }
            else if (key == godot::KEY_HOME) {
                BROWSER_DEBUG_VAL("KEY_HOME");
                event.windows_key_code = 36; // Debut
            }
            else if (key == godot::KEY_END) {
                BROWSER_DEBUG_VAL("KEY_END");
                event.windows_key_code = 35; // Fin
            }
            else if (key == godot::KEY_INSERT) {
                BROWSER_DEBUG_VAL("KEY_INSERT");
                event.windows_key_code = 45; // Insert
            }
            else if (key == godot::KEY_DELETE) {
                BROWSER_DEBUG_VAL("KEY_DELETE");
                event.windows_key_code = 46; // Del (not dot when no char event)
            }

            event.type = KEYEVENT_KEYDOWN;
            event.character = char16_t(event.windows_key_code);
            event.native_key_code = event.windows_key_code;
            m_browser->GetHost()->SendKeyEvent(event);
        }
        else
        {
            BROWSER_DEBUG_VAL("Any Char");
            event.windows_key_code = key;
            event.character = key16b;
            event.native_key_code = key;
            event.unmodified_character = key16b;

            event.type = KEYEVENT_KEYDOWN;
            m_browser->GetHost()->SendKeyEvent(event);
            event.type = pressed ? KEYEVENT_CHAR : KEYEVENT_KEYUP;
            m_browser->GetHost()->SendKeyEvent(event);
        }
    }
    else
    {
        BROWSER_DEBUG_VAL("PRESSED FALSE");
        event.native_key_code |= int(0xC0000000);
        event.type = KEYEVENT_KEYUP;
        m_browser->GetHost()->SendKeyEvent(event);
    }
}
