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

#ifndef STIGMEE_GDCEF_HPP
#define STIGMEE_GDCEF_HPP

// ****************************************************************************
// previously we derived from ImageTexture the core .h files from godot.
// in GDNative we can directly use the corresponding .hpp file is the class has
// been exposed (most of them are). so simply include ImageTexture.hpp
// PoolVector doe not seem to be exposed but we can use PoolByteArray native type instead.
// Most of the code replicates what's been done previously by @lecrapouille in the
// ****************************************************************************

// Godot
#include <Godot.hpp>
#include <GDScript.hpp>
#include <Node.hpp>
#include <ImageTexture.hpp>

// Chromium Embedded Framework
#include "cef_render_handler.h"
#include "cef_client.h"
#include "cef_app.h"

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

// ****************************************************************************
//! \brief Class deriving from Godot's Node and interfacing Chromium Embedded
//! Framework.
// ****************************************************************************
class GDCef : public godot::Node
{
    GODOT_CLASS(GDCef, godot::Node);

public:

    //! \brief Default Constructor. Initialize internal states. Nothing else is
    //! made because Godot engine will automatically call the _init() method.
    GDCef();

    //! \brief Destructor. Release CEF memory and sub CEF processes are notified
    //! that the application is exiting.
    ~GDCef();

    //! \brief Return the Godot texture holding the page content to other Godot
    //! element that needs it for the rendering.
    //! \fixme FIXME Need mutex ?
    godot::Ref<godot::ImageTexture> texture()
    {
        return m_texture;
    }

    //! \brief Pomp messages with other sub CEF processes.
    //! \pre Shall be called periodically.
    //! \fixme FIXME Need to be a static method ? Can we run it inside a thread ?
    void doMessageLoopWork();

    //! \brief Set the render zoom level
    void setZoomLevel(double delta);

    //! \brief Load the given web page
    void loadURL(godot::String url); // FIXME mmmh by copy really ? Not godot::String const& url ?

    //! \brief Get the current url of the browser
    godot::String getUrl();

    //! \brief Navigate to the previous page if possible
    void navigateBack();

    //! \brief Navigate to the next page if possible
    void navigateForward();

    //! \brief Set the windows size
    void reshape(int w, int h);

    //! \brief TODO
    // void executeJS(const std::string &cmd);

    //! \brief Down then up on Left button
    void leftClick();

    //! \brief Down then up on Right button
    void rightClick();

    //! \brief Down then up on middle button
    void middleClick();

    //! \brief Left Mouse button up
    void leftMouseUp();

    //! \brief Right Mouse button up
    void rightMouseUp();

    //! \brief Middle Mouse button up
    void middleMouseUp();

    //! \brief Left Mouse button down
    void leftMouseDown();

    //! \brief Right Mouse button down
    void rightMouseDown();

    //! \brief Middle Mouse button down
    void middleMouseDown();

    //! \brief Set the new mouse position.
    void mouseMove(int x, int y);

    //! \brief run Mouse Wheel
    void mouseWheel(const int wDelta);

    //! \brief Set the new keyboard state (char typed ...)
    void keyPress(int key, bool pressed, bool shift, bool alt, bool ctrl);

    //! \brief Our initializer called by Godot
    void _init();

    //! \brief Static function that Godot will call to find out which methods
    //! can be called on our NativeScript and which properties it exposes.
    static void _register_methods();

private:

    //! \brief Return the browser or create one if needed. This allows to postponed
    CefRefPtr<CefBrowser> browser(godot::String url = "https://labo.stigmee.fr");

    // *************************************************************************
    //! \brief Manager.
    // *************************************************************************
    class Manager : public CefApp
    {
    public:

        virtual void OnBeforeCommandLineProcessing(
            const CefString& ProcessType, CefRefPtr<CefCommandLine> CommandLine) override;

        static CefSettings Settings;
        static CefMainArgs MainArgs;
        static bool CPURenderSettings;
        static bool AutoPlay;

        IMPLEMENT_REFCOUNTING(GDCef::Manager);
    };

    // *************************************************************************
    //! \brief Private implementation to handle CEF events to draw the web page.
    // *************************************************************************
    class RenderHandler : public CefRenderHandler
    {
    public:

        RenderHandler(GDCef& owner);

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

        //! \brief Browser's view dimension.
        //! Initial browser's view size. We expose it to Godot which can set the
        //! desired size depending on its viewport size.
        int m_width = 128;
        int m_height = 128;

        //! \brief Access to GDCef::m_image
        GDCef& m_owner;

        //! \brief
        godot::PoolByteArray m_data;
    };

    // *************************************************************************
    //! \brief Provide access to browser-instance-specific callbacks. A single
    //! CefClient instance can be shared among any number of browsers.
    // *************************************************************************
    class BrowserClient : public CefClient, public CefLifeSpanHandler
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

        // CefLifeSpanHandler
        virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
        {
            // CEF_REQUIRE_UI_THREAD();

            if (!m_browser.get())
            {
                // Keep a reference to the main browser.
                m_browser = browser;
                m_browser_id = browser->GetIdentifier();
            }
        }

        // CefLifeSpanHandler
        virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
        {
            // CEF_REQUIRE_UI_THREAD();

            if (m_browser_id == browser->GetIdentifier())
            {
                m_browser = nullptr;
            }
        }

        CefRefPtr<CefBrowser> GetCEFBrowser()
        {
            return m_browser;
        }

        IMPLEMENT_REFCOUNTING(BrowserClient);

    private:

        CefRefPtr<CefRenderHandler> m_renderHandler;
        CefRefPtr<CefBrowser> m_browser;
        int m_browser_id;
    };

private:

    //! \brief Chromium Embedded Framework elements
    CefRefPtr<CefBrowser> m_browser;
    CefRefPtr<BrowserClient> m_client;
    RenderHandler* m_render_handler = nullptr;

    //! \brief Mouse cursor position on the main window
    int m_mouse_x;
    int m_mouse_y;

    //! \brief Godot's temporary image (CEF => Godot)
    godot::Ref<godot::ImageTexture> m_texture;
    godot::Ref<godot::Image> m_image;

    //! \brief Various browser settings.
    CefBrowserSettings m_settings;
    CefWindowInfo m_window_info;
};

#endif
