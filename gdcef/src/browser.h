//*************************************************************************
// Stigmee: A 3D browser and decentralized social network.
// Copyright 2021 Alain Duron <duron.alain@gmail.com>
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

#ifndef BROWSERVIEW_H
#define BROWSERVIEW_H

// ****************************************************************************
// previously we derived from ImageTexture the core .h files from godot.
// in GDNative we can directly use the corresponding .hpp file is the class has
// been exposed (most of them are). so simply include ImageTexture.hpp
// PoolVector doe not seem to be exposed but we can use PoolByteArray native type instead.
// Most of the code replicates what's been done previously by @lecrapouille in the
// ****************************************************************************

#include <Godot.hpp>
#include <Node.hpp>
#include <Image.hpp>
#include <ImageTexture.hpp>

// Chromium Embedded Framework
#include "./include/cef_render_handler.h"
#include "./include/cef_client.h"
#include "./include/cef_app.h"

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

class BrowserView : public godot::Node
{
private:

    GODOT_CLASS(BrowserView, godot::Node);

public:

    static void _register_methods();

    //! \brief Default Constructor.
    BrowserView(/*const String &url*/);

    //! \brief
    ~BrowserView();

    void _init(); // our initializer called by Godot

    godot::Ref<godot::ImageTexture> get_texture() { return m_texture; }

    //! \brief Load the given web page
    void load_url(const godot::String& url);

    //! \brief Set the windows size
    void reshape(int w, int h);

    //! \brief TODO
    // void executeJS(const std::string &cmd);

    //! \brief Set the new mouse position.
    void mouseMove(int x, int y);

    //! \brief Set the new mouse state (clicked ...)
    void mouseClick(int button, bool mouse_up);

    //! \brief Set the new keyboard state (char typed ...)
    void keyPress(int key, bool pressed);

private:

    // *************************************************************************
    //! \brief Private implementation to handle CEF events to draw the web page.
    // *************************************************************************
    class RenderHandler : public CefRenderHandler
    {
    public:

        RenderHandler(BrowserView& owner);

        //! \brief Resize the browser's view
        void reshape(int w, int h);

        //! \brief CefRenderHandler interface. Get the view port.
        virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

        //! \brief CefRenderHandler interface. Update the Godot's texture.
        virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
                             const RectList& dirtyRects, const void* buffer,
                             int width, int height) override;

        //! \brief CefBase interface
        IMPLEMENT_REFCOUNTING(RenderHandler);

    private:

        //! \brief Browser's view dimension
        int m_width;
        int m_height;

        //! \brief Access to BrowserView::m_image
        BrowserView& m_owner;

        //! \brief
        //PoolVector<uint8_t> m_data;
        godot::PoolByteArray m_data;
    };

    // *************************************************************************
    //! \brief Provide access to browser-instance-specific callbacks. A single
    //! CefClient instance can be shared among any number of browsers.
    // *************************************************************************
    class BrowserClient : public CefClient//, public CefLifeSpanHandler
    {
    public:

        BrowserClient(CefRefPtr<CefRenderHandler> ptr)
            : m_renderHandler(ptr)
        {}

        // CefClient
        virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override
        {
            return m_renderHandler;
        }

#if 0
        // CefLifeSpanHandler
        virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
        {
            return this;
        }

        // CefLifeSpanHandler
        virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
        {
            CEF_REQUIRE_UI_THREAD();
            m_browser = browser;
        }

        // CefLifeSpanHandler
        virtual bool DoClose(CefRefPtr<CefBrowser> browser) override
        {
            CEF_REQUIRE_UI_THREAD();
            return true;
        }

        // CefLifeSpanHandler
        virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
        {}
#endif

        CefRefPtr<CefRenderHandler> m_renderHandler;

        IMPLEMENT_REFCOUNTING(BrowserClient);
    };

private:

    //! \brief Chromium Embedded Framework elements
    CefRefPtr<CefBrowser> m_browser;
    CefRefPtr<BrowserClient> m_client;
    RenderHandler* m_render_handler = nullptr;
    HWND m_handle;

    //! \brief Mouse cursor position on the main window
    int m_mouse_x;
    int m_mouse_y;

    //! \brief Godot's temporary image (CEF => Godot)
    godot::Ref<godot::ImageTexture> m_texture;
    godot::Ref<godot::Image> m_image;
};

#endif
