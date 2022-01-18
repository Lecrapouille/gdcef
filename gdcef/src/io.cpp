//*************************************************************************
// Stigmee: The art to sanctuarize knowledge exchanges.
// Copyright 2021-2022 Alain Duron <duron.alain@gmail.com>
// Copyright 2021-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Stigmee.
//
// Stigmee is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//*************************************************************************

#include "gdcef.h"
#include <GlobalConstants.hpp> // Godot

//------------------------------------------------------------------------------
void GDCef::mouseMove(int x, int y)
{
    if (m_browser == nullptr)
        return ;

    m_mouse_x = x;
    m_mouse_y = y;

    CefMouseEvent evt;
    evt.x = x;
    evt.y = y;

    bool mouse_leave = false; // TODO
    // AD - Adding focus just like what's done in BLUI
    auto host = m_browser->GetHost();
    host->SetFocus(true);
    host->SendMouseMoveEvent(evt, mouse_leave);
}

//------------------------------------------------------------------------------
void GDCef::mouseClick(int button, bool mouse_up)
{
    if (m_browser == nullptr)
        return ;

    std::cout << "[GDCEF] [GDCef::mouseClick] mouse event occured" << std::endl;
    CefMouseEvent evt;
    std::cout << "[GDCEF] [GDCef::mouseClick] x,y" << m_mouse_x << "," << m_mouse_y << std::endl;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;

    std::cout << "[GDCEF] [GDCef::mouseClick]" << std::endl;

    CefBrowserHost::MouseButtonType btn;
    switch (button)
    {
    case godot::GlobalConstants::BUTTON_LEFT:
        btn = CefBrowserHost::MouseButtonType::MBT_LEFT;
        std::cout << "[GDCEF] [GDCef::mouseClick] Left " << mouse_up << std::endl;
        break;
    case godot::GlobalConstants::BUTTON_RIGHT:
        btn = CefBrowserHost::MouseButtonType::MBT_RIGHT;
        std::cout << "[GDCEF] [GDCef::mouseClick] Right " << mouse_up << std::endl;
        break;
    case godot::GlobalConstants::BUTTON_MIDDLE:
        btn = CefBrowserHost::MouseButtonType::MBT_MIDDLE;
        std::cout << "[GDCEF] [GDCef::mouseClick] Middle " << mouse_up << std::endl;
        break;
    default:
        return;
    }

    int click_count = 1; // TODO
    m_browser->GetHost()->SendMouseClickEvent(evt, btn, mouse_up, click_count);
}

//------------------------------------------------------------------------------
void GDCef::mouseWheel(const int wDelta)
{
    if (m_browser == nullptr)
        return ;

    std::cout << "[GDCEF] [GDCef::mouseWheel] mouse wheel rolled" << std::endl;
    CefMouseEvent evt;
    std::cout << "[GDCEF] [GDCef::mouseWheel] x,y,wDelta : [" << m_mouse_x << ","
              << m_mouse_y << "," << wDelta << "]" << std::endl;

    evt.x = m_mouse_x;
    evt.y = m_mouse_y;

    m_browser->GetHost()->SendMouseWheelEvent(evt, wDelta * 10, wDelta * 10);
}

//------------------------------------------------------------------------------
void GDCef::keyPress(int key, bool pressed, bool isup)
{
    if (m_browser == nullptr)
        return ;

    // Not working yet, need some focus implementation
    CefKeyEvent evtdown;
    CefKeyEvent evtup;

    evtdown.character = key;
    evtdown.windows_key_code = key;
    evtdown.native_key_code = key;
    evtdown.type = pressed ? KEYEVENT_CHAR : KEYEVENT_KEYDOWN;

    evtup.character = key;
    evtup.windows_key_code = key;
    evtup.native_key_code = key;
    evtup.type = KEYEVENT_KEYUP;

    auto host = m_browser->GetHost();
    // host->SetFocus(true);
    if (pressed)
    {
        host->SendKeyEvent(evtdown);
        host->SendKeyEvent(evtup);
    }
    else
    {
        if (isup)
        {
            host->SendKeyEvent(evtup);
        }
        else
        {
            host->SendKeyEvent(evtdown);
        }
    }
}
