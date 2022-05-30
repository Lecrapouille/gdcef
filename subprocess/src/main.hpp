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

#ifndef GDCEF_SUBPROCESS_CLIENT_HPP
#  define GDCEF_SUBPROCESS_CLIENT_HPP

#  include <list>
#  include <iostream>

// ----------------------------------------------------------------------------
// Suppress warnings
// ----------------------------------------------------------------------------
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
#      if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wundef"
#        pragma clang diagnostic ignored "-Wshadow-field"
#        pragma clang diagnostic ignored "-Wcast-qual"
#      endif
#  endif

#  include "cef_client.h"
#  include "cef_app.h"
#  include "cef_browser.h"
#  include "wrapper/cef_helpers.h"
#  ifdef __APPLE__
#    include "include/wrapper/cef_library_loader.h"
#  endif

// *****************************************************************************
//! \brief
// *****************************************************************************
class GDCefClient : public CefClient,
                    public CefLifeSpanHandler,
                    public CefDisplayHandler
{
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
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
    {
        std::cout << "[SubProcess] [GDCefClient::OnAfterCreated]" << std::endl;
        CEF_REQUIRE_UI_THREAD();

        // Add to the list of existing browsers.
        m_browser_list.push_back(browser);
    }

        // -------------------------------------------------------------------------
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
    {
        std::cout << "[SubProcess] [GDCefClient::OnBeforeClose]" << std::endl;
        CEF_REQUIRE_UI_THREAD();

        // Remove from the list of existing browsers.
        BrowserList::iterator bit = m_browser_list.begin();
        for (; bit != m_browser_list.end(); ++bit)
        {
            if ((*bit)->IsSame(browser))
            {
                m_browser_list.erase(bit);
                break;
            }
        }

        if (m_browser_list.empty())
        {
            // All browser windows have closed. Quit the application message loop.
            CefQuitMessageLoop();
        }
    }

private:

    // -------------------------------------------------------------------------
    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(GDCefClient);

private:

    using BrowserList = std::list<CefRefPtr<CefBrowser>>;
    BrowserList m_browser_list;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
class GDCefBrowser : public CefApp,
                     public CefBrowserProcessHandler
{
private: // CefApp methods

    // -------------------------------------------------------------------------
    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
    {
        return this;
    }

private: // CefBrowserProcessHandler methods

    // -------------------------------------------------------------------------
    virtual void OnContextInitialized() override
    {
        std::cout << "[SubProcess] [GDCefBrowser::OnContextInitialized] begin" << std::endl;
        CEF_REQUIRE_UI_THREAD();

        // Information used when creating the native window.
        CefWindowInfo window_info;

#if defined(OS_WIN)
        // On Windows we need to specify certain flags that will be passed to
        // CreateWindowEx().
        window_info.SetAsPopup(NULL, "CEF");
#endif

        // GDCefClient implements browser-level callbacks.
        std::cout << "[SubProcess] [GDCefBrowser::OnContextInitialized] Create client handler" << std::endl;
        CefRefPtr<GDCefClient> handler(new GDCefClient());

        // Specify CEF browser settings here.
        CefBrowserSettings browser_settings;

        // Create the first browser window.
        std::cout << "[SubProcess] [GDCefBrowser::OnContextInitialized] Create the browser" << std::endl;
        CefBrowserHost::CreateBrowser(window_info, handler.get(), "https://mediatheque.stigmee.fr",
                                      browser_settings, nullptr, nullptr);
    }

private:

    IMPLEMENT_REFCOUNTING(GDCefBrowser);
};

#  if !defined(_WIN32)
#      if defined(__clang__)
#        pragma clang diagnostic pop
#      endif
#    pragma GCC diagnostic pop
#  endif

#endif // GDCEF_SUBPROCESS_CLIENT_HPP
