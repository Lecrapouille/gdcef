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

#include "render_process.hpp"

#define DEBUG_RENDER_PROCESS(txt)                                       \
    {                                                                   \
        std::stringstream ss;                                           \
        ss << "\033[32m[Secondary Process][RenderProcess::" << __func__ \
           << "] " << txt << "\033[0m";                                 \
        std::cout << ss.str() << std::endl;                             \
    }

#define DEBUG_BROWSER_PROCESS(txt)                                             \
    {                                                                          \
        std::stringstream ss;                                                  \
        ss << "\033[32m[Secondary Process][GDCefBrowser::" << __func__ << "] " \
           << txt << "\033[0m";                                                \
        std::cout << ss.str() << std::endl;                                    \
    }

//------------------------------------------------------------------------------
RenderProcess::~RenderProcess()
{
    DEBUG_RENDER_PROCESS("");
}

#if 0
//------------------------------------------------------------------------------
void RenderProcess::OnContextInitialized()
{
    CEF_REQUIRE_UI_THREAD();
    DEBUG_RENDER_PROCESS("");

    // Information used when creating the native window.
    CefWindowInfo window_info;

#    if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    window_info.SetAsPopup(NULL, "CEF");
#    endif

    // GDCefBrowser implements browser-level callbacks.
    DEBUG_RENDER_PROCESS("Create client handler");
    CefRefPtr<GDCefBrowser> handler(new GDCefBrowser());

    // Specify CEF browser settings here.
    CefBrowserSettings browser_settings;

    // Create the first browser window.
    DEBUG_RENDER_PROCESS("Create the browser");
    CefBrowserHost::CreateBrowser(
        window_info, handler.get(), "", browser_settings, nullptr, nullptr);
}
#endif

//------------------------------------------------------------------------------
void RenderProcess::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context)
{
    DEBUG_RENDER_PROCESS("");
}

#if 0
//------------------------------------------------------------------------------
GDCefBrowser::~GDCefBrowser()
{
    DEBUG_BROWSER_PROCESS("");
}

//------------------------------------------------------------------------------
void GDCefBrowser::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    DEBUG_BROWSER_PROCESS("");

    // Add to the list of existing browsers.
    m_browser_list.push_back(browser);
}

//------------------------------------------------------------------------------
void GDCefBrowser::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    DEBUG_BROWSER_PROCESS("");

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
        DEBUG_BROWSER_PROCESS("CefQuitMessageLoop");
        // All browser windows have closed.
        // Quit the application message loop.
        CefQuitMessageLoop();
    }
}
#endif