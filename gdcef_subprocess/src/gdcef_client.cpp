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

#include "gdcef_client.hpp"

// CEF
#include <cef_app.h>
#include <cef_client.h>
#include <cef_render_handler.h>
#include <cef_life_span_handler.h>
#include <cef_load_handler.h>
#include <cef_parser.h>
#include <views/cef_browser_view.h>
#include <views/cef_window.h>
#include <wrapper/cef_helpers.h>
#include <wrapper/cef_closure_task.h>
#include <internal/cef_types.h>
#include <base/cef_bind.h>
#include <base/cef_callback.h>

#include <sstream>
#include <iostream>
#include <string>


#if 0

// No idea why but can't get ERR_ABORTED without this.
// Namespace issue ?
typedef enum {
#define NET_ERROR(label, value) ERR_ ## label = value,
#include <base/internal/cef_net_error_list.h>
#undef NET_ERROR
} cef_app_errorcode_t;

#endif

static GDCefClient* g_instance = nullptr;

// Returns a data: URI with the specified contents.
static std::string GetDataURI(const std::string& data, const std::string& mime_type)
{
    return "data:" + mime_type + ";base64," +
            CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
            .ToString();
}

GDCefClient::GDCefClient()
    : m_is_closing(false)
{
    std::cout << "[SubProcess] [GDCefClient::GDCefClient]" << std::endl;
    DCHECK(!g_instance);
    g_instance = this;
}

GDCefClient::~GDCefClient()
{
    std::cout << "[SubProcess] [GDCefClient::~GDCefClient]" << std::endl;
    g_instance = nullptr;
}

// static
GDCefClient* GDCefClient::GetInstance()
{
    return g_instance;
}

void GDCefClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    std::cout << "[SubProcess] [GDCefClient::OnAfterCreated]" << std::endl;
    CEF_REQUIRE_UI_THREAD();

    // Add to the list of existing browsers.
    m_browser_list.push_back(browser);
}

bool GDCefClient::DoClose(CefRefPtr<CefBrowser> browser)
{
    std::cout << "[SubProcess] [GDCefClient::DoClose]" << std::endl;
    CEF_REQUIRE_UI_THREAD();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    if (m_browser_list.size() == 1)
    {
        // Set a flag to indicate that the window close should be allowed.
        m_is_closing = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void GDCefClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
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

void GDCefClient::OnLoadError(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              ErrorCode errorCode,
                              const CefString& errorText,
                              const CefString& failedUrl)
{
    std::cout << "[SubProcess] [GDCefClient::OnLoadError]" << std::endl;
    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    CEF_REQUIRE_UI_THREAD();

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
            "<h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText)
       << " (" << errorCode << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void GDCefClient::CloseAllBrowsers(bool force_close)
{
    std::cout << "[SubProcess] [GDCefClient::CloseAllBrowsers]" << std::endl;
    if (m_browser_list.empty())
        return;

    if (!CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::BindOnce(&GDCefClient::CloseAllBrowsers, this,
                                           force_close));
        return;
    }

    BrowserList::const_iterator it = m_browser_list.begin();
    for (; it != m_browser_list.end(); ++it)
    {
        (*it)->GetHost()->CloseBrowser(force_close);
    }
}
