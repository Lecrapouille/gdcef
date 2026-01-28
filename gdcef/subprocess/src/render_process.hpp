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

#ifndef GDCEF_RENDER_PROCESS_HPP
#define GDCEF_RENDER_PROCESS_HPP

#include <iostream>
#include <list>

// ----------------------------------------------------------------------------
// Suppress warnings
// ----------------------------------------------------------------------------
#if !defined(_WIN32)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#    pragma GCC diagnostic ignored "-Wparentheses"
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#    pragma GCC diagnostic ignored "-Wfloat-conversion"
#    pragma GCC diagnostic ignored "-Wfloat-equal"
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wshadow"
#    if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wundef"
#        pragma clang diagnostic ignored "-Wshadow-field"
#        pragma clang diagnostic ignored "-Wcast-qual"
#    endif
#endif

#include "cef_app.h"
#include "cef_browser.h"
#include "cef_client.h"
#include "wrapper/cef_helpers.h"

#ifdef __APPLE__
#    include "include/wrapper/cef_library_loader.h"
#endif

// *****************************************************************************
//! \brief Class called by the JavaScript layer to call a GDScript method.
// *****************************************************************************
class GodotMethodHandler: public CefV8Handler
{
public:

    // -------------------------------------------------------------------------
    //! \brief Constructor
    //! \param[in] browser The browser instance
    // -------------------------------------------------------------------------
    GodotMethodHandler(CefRefPtr<CefBrowser> browser) : m_browser(browser) {}

    // -------------------------------------------------------------------------
    //! \brief Called to execute a Godot method.
    //! Send an IPC message to the main process to execute the Godot method.
    //! The GDBrowserView::onProcessMessageReceived method will be called.
    //! \param[in] name The method name.
    //! \param[in] object The object.
    //! \param[in] arguments The arguments.
    //! \param[out] retval The return value.
    //! \param[out] exception The exception.
    // -------------------------------------------------------------------------
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception);

    IMPLEMENT_REFCOUNTING(GodotMethodHandler);

private:

    //------------------------------------------------------------------------------
    //! \brief Convert a V8 value to a JSON string
    //! \param[in] value The V8 value to convert
    //! \return The JSON string representation
    //------------------------------------------------------------------------------
    std::string V8ToJSON(CefRefPtr<CefV8Value> value);

    CefRefPtr<CefBrowser> m_browser;
};

// *****************************************************************************
//! \brief Entry point for the render process
// *****************************************************************************
class RenderProcess: public CefApp, public CefRenderProcessHandler
{
public:

    IMPLEMENT_REFCOUNTING(RenderProcess);

    // -------------------------------------------------------------------------
    //! \brief Called when the IPC message is received from the main process.
    //! Used here to execute JS code.
    //! \param[in] browser The browser instance.
    //! \param[in] frame The frame instance.
    //! \param[in] source_process The source process.
    //! \param[in] message The message.
    // -------------------------------------------------------------------------
    virtual bool
    OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefProcessId source_process,
                             CefRefPtr<CefProcessMessage> message) override;

private: // CefApp methods

    // -------------------------------------------------------------------------
    virtual CefRefPtr<CefRenderProcessHandler>
    GetRenderProcessHandler() override
    {
        return this;
    }

private: // CefRenderProcessHandler methods

    // -------------------------------------------------------------------------
    //! \brief Called when a new context is created.
    //! Add JS code to the context to allow communication between JS and
    //! GDScript.
    //! \param[in] browser The browser instance.
    //! \param[in] frame The frame instance.
    //! \param[in] context The context instance.
    // -------------------------------------------------------------------------
    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) override;

    // -------------------------------------------------------------------------
    //! \brief Called when a context is released. Clean JS/Godot bindings.
    //! \param[in] browser The browser instance.
    //! \param[in] frame The frame instance.
    //! \param[in] context The context instance.
    // -------------------------------------------------------------------------
    virtual void OnContextReleased(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefV8Context> context) override;

private:

    CefRefPtr<GodotMethodHandler> m_handler;
    CefRefPtr<CefV8Context> m_context;
    CefRefPtr<CefFrame> m_frame;
};

#if !defined(_WIN32)
#    if defined(__clang__)
#        pragma clang diagnostic pop
#    endif
#    pragma GCC diagnostic pop
#endif

#endif // GDCEF_RENDER_PROCESS_HPP
