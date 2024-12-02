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
//! \brief Entry point for the render process
// *****************************************************************************
class RenderProcess: public CefApp,
                     // public CefBrowserProcessHandler,
                     public CefRenderProcessHandler
{
public:

    ~RenderProcess();

private: // CefApp methods

    // -------------------------------------------------------------------------
    // virtual CefRefPtr<CefBrowserProcessHandler>
    // GetBrowserProcessHandler() override
    // {
    //    return this;
    //}

    virtual CefRefPtr<CefRenderProcessHandler>
    GetRenderProcessHandler() override
    {
        return this;
    }

private: // CefBrowserProcessHandler methods

    // -------------------------------------------------------------------------
    // virtual void OnContextInitialized() override;

private: // CefRenderProcessHandler methods

    // -------------------------------------------------------------------------
    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) override;

private:

    IMPLEMENT_REFCOUNTING(RenderProcess);
};

#if 0
// *****************************************************************************
//! \brief Browser process handler
// *****************************************************************************
class GDCefBrowser: public CefClient,
                    public CefLifeSpanHandler,
                    public CefDisplayHandler
{
public:

    ~GDCefBrowser();

private: // CefDisplayHandler methods

    // -------------------------------------------------------------------------
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
    {
        return this;
    }

private: // CefLifeSpanHandler methods

    // -------------------------------------------------------------------------
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
    {
        return this;
    }

    // -------------------------------------------------------------------------
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

    // -------------------------------------------------------------------------
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

private:

    // -------------------------------------------------------------------------
    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(GDCefBrowser);

private:

    using BrowserList = std::list<CefRefPtr<CefBrowser>>;
    BrowserList m_browser_list;
};
#endif

#if !defined(_WIN32)
#    if defined(__clang__)
#        pragma clang diagnostic pop
#    endif
#    pragma GCC diagnostic pop
#endif

#endif // GDCEF_RENDER_PROCESS_HPP
