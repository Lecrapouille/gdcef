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

#ifndef STIGMEE_GDCEF_HPP
#  define STIGMEE_GDCEF_HPP

// *****************************************************************************
//! \file Wrap Chromium Embedded Framework (that can be find at
//! $WORKSPACE_STIGMEE/godot/gdnative/browser/thirdparty/cef_binary) as Godot
//! native module
// *****************************************************************************

// Hide compilation warnings induced by Godot and by CEF
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
#  include "OS.hpp"
#  include "Node.hpp"
#  include "ImageTexture.hpp"

// Chromium Embedded Framework
#  include "cef_client.h"
#  include "cef_app.h"

class GDBrowserView;

// *****************************************************************************
//! \brief Class deriving from Godot's Node and interfacing Chromium Embedded
//! Framework. This class can create isntances of GDBrowserView and manage their
//! lifetime.
// *****************************************************************************
class GDCef : public godot::Node
{
public: // Godot interfaces.

    // -------------------------------------------------------------------------
    //! \brief Mandatory initializer automatically called by Godot.
    // -------------------------------------------------------------------------
    void _init();

    // -------------------------------------------------------------------------
    //! \brief Method automatically called by Godot engine to register the
    //! desired C++ methods that will be callable from gdscript.
    // -------------------------------------------------------------------------
    static void _register_methods();

    // -------------------------------------------------------------------------
    //! \brief Process automatically called by Godot engine. Call the CEF pomp
    //! loop message.
    // -------------------------------------------------------------------------
    void _process(float delta);

private: // Godot interfaces.

    // -------------------------------------------------------------------------
    //! \brief Godot reference counting. Beware can conflict with CEF reference
    //! counting: this is why wehave to implement the sub class Impl.
    // -------------------------------------------------------------------------
    GODOT_CLASS(GDCef, godot::Node);

private: // CEF interfaces.

    // *************************************************************************
    //! \brief Mandatory since Godot reference counter is conflicting with CEF
    //! reference counting and therefore we reach with pure virtual destructor
    //! called. To avoid this crash, we have to create this intermediate class.
    // *************************************************************************
    class Impl: public CefLifeSpanHandler,
                public CefClient
    {
    public:

        // ---------------------------------------------------------------------
        //! \brief Default constructor getting the owner of the class instance.
        // ---------------------------------------------------------------------
        Impl(GDCef& cef)
            : m_owner(cef)
        {}

        // ---------------------------------------------------------------------
        //! \brief Should be called on the main application thread to shut down
        //! the CEF browser process before the application exits.
        // ---------------------------------------------------------------------
        virtual ~Impl()
        {
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
    //! container. Return the browser identifier or return nullptr in case of failure.
    //! \param[in] url the page link (by copy needed by Godot).
    //! \param[in] name the browser name (by copy needed by Godot).
    //! \param[in] w the width dimension of the document.
    //! \param[in] h the height dimension of the document.
    //! \param[in] Return the address of the newly created browser (or nullptr
    //! in case of error).
    // -------------------------------------------------------------------------
    GDBrowserView* createBrowser(godot::String const url, godot::String const name,
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
