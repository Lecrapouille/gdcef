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
#  define STIGMEE_GDCEF_HPP

// Hide compilation warnings induced by Godot and CEF
#  if !defined(_WIN32)
#    pragma GCC diagnostic push
#      pragma GCC diagnostic ignored "-Wold-style-cast"
#      pragma GCC diagnostic ignored "-Wparentheses"
#      pragma GCC diagnostic ignored "-Wunused-parameter"
#      pragma GCC diagnostic ignored "-Wconversion"
#      pragma GCC diagnostic ignored "-Wsign-conversion"
#      pragma GCC diagnostic ignored "-Wfloat-conversion"
#      pragma GCC diagnostic ignored "-Wfloat-equal"
#      pragma GCC diagnostic ignored "-Wpedantic"
#      pragma GCC diagnostic ignored "-Wshadow"
#      pragma GCC diagnostic ignored "-Wcast-qual"
#      if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wundef"
#        pragma clang diagnostic ignored "-Wshadow-field"
#      endif
#  endif

// Godot
#  include "Godot.hpp"
#  include "GDScript.hpp"
#  include "Node.hpp"
#  include "ImageTexture.hpp"
#  include "GlobalConstants.hpp"

// Chromium Embedded Framework
#  include "cef_render_handler.h"
#  include "cef_client.h"
#  include "cef_app.h"
#  include "wrapper/cef_helpers.h"

#include <iostream>

class BrowserView;

// *****************************************************************************
//! \brief Class deriving from Godot's Node and interfacing Chromium Embedded
//! Framework. This class can create isntances of BrowserView and manage their
//! lifetime.
// *****************************************************************************
class GDCef : public godot::Node
{
public: // Godot interfaces.

    // -------------------------------------------------------------------------
    //! \brief Our initializer called by Godot.
    // -------------------------------------------------------------------------
    void _init();

    // -------------------------------------------------------------------------
    //! \brief Static function that Godot will call to find out which methods
    //! can be called on our NativeScript and which properties it exposes.
    // -------------------------------------------------------------------------
    static void _register_methods();

    // -------------------------------------------------------------------------
    //! \brief Call CEF pomp loop message
    // -------------------------------------------------------------------------
    void _process(float delta);

    // -------------------------------------------------------------------------
    //! \brief Godot stuff
    // -------------------------------------------------------------------------
    GODOT_CLASS(GDCef, godot::Node);

private: // CEF interfaces.

    // *************************************************************************
    //! \brief Mandatory since Godot ref counter is conflicting with CEF ref
    //! counting and therefore we reach with pure virtual destructor called.
    //! To avoid this we have to create this intermediate class.
    // *************************************************************************
    class Impl: public CefLifeSpanHandler,
                public CefClient
    {
    public:

        // ---------------------------------------------------------------------
        //! \brief Pass the owner instance.
        // ---------------------------------------------------------------------
        Impl(GDCef& view)
            : m_owner(view)
        {}

        virtual ~Impl()
        {
            std::cout << "GDCef::Impl::~Impl" << std::endl;
            CefShutdown();
        }

    private: // CefClient::CefBaseRefCounted interfaces.

        // ---------------------------------------------------------------------
        //! \brief CEF reference couting
        // ---------------------------------------------------------------------
        IMPLEMENT_REFCOUNTING(Impl);

    private: // CefClient interfaces

        // ---------------------------------------------------------------------
        //! \brief Return the handler for browser life span events.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
        {
            return this;
        }

    private: // CefLifeSpanHandler interfaces

        virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
        virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
        virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    private:

        GDCef& m_owner;
    };

public:

    GDCef();

    // -------------------------------------------------------------------------
    //! \brief Destructor. Release CEF memory and sub CEF processes are notified
    //! that the application is exiting. All browsers are destroyed.
    // -------------------------------------------------------------------------
    ~GDCef();

    // -------------------------------------------------------------------------
    //! \brief Allow Godot script to release CEF.
    // -------------------------------------------------------------------------
    void shutdown();

    // -------------------------------------------------------------------------
    //! \brief Const getter CEF settings.
    // -------------------------------------------------------------------------
    inline CefSettings const& settingsCEF() const
    {
        return m_cef_settings;
    }

    // -------------------------------------------------------------------------
    //! \brief Const getter browser settings.
    // -------------------------------------------------------------------------
    inline CefBrowserSettings const& settingsBrowser() const
    {
        return m_browser_settings;
    }

    // -------------------------------------------------------------------------
    //! \brief Const getter browser window settings.
    // -------------------------------------------------------------------------
    inline CefWindowInfo const& windowInfo() const
    {
        return m_window_info;
    }

    // -------------------------------------------------------------------------
    //! \brief Create a browser view and store its instance inside the internal
    //! container. Return the browser identifier or return -1 in case of failure.
    //! \param[in] url the page link (by copy needed by Godot).
    //! \param[in] name the browser name (by copy needed by Godot).
    //! \param[in] w the width dimension of the document.
    //! \param[in] h the height dimension of the document.
    //! \param[in] Return the address of the newly created browser (or nullptr
    //! in case of error).
    // -------------------------------------------------------------------------
    BrowserView* createBrowser(godot::String const url, godot::String const name,
                               int w, int h);

private:

    //! \brief CEF interface implementation
    friend GDCef::Impl;
    //! \brief CEF interface implementation
    CefRefPtr<GDCef::Impl> m_impl = nullptr;
    //! \brief
    CefWindowInfo m_window_info;
    //! \brief
    CefSettings m_cef_settings;
    //! \brief
    CefBrowserSettings m_browser_settings;
};

#  if !defined(_WIN32)
#      if defined(__clang__)
#        pragma clang diagnostic pop
#      endif
#    pragma GCC diagnostic pop
#  endif

#endif
