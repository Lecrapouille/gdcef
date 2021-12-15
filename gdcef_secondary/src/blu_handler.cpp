// This code is a modification of the original projects that can be found at
// https://github.com/ashea-code/BluBrowser

#include "blu_handler.h"

#include "include/base/cef_bind.h"
#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#include <sstream>
#include <string>

static BluHandler* g_instance = nullptr;

// Returns a data: URI with the specified contents.
static std::string GetDataURI(const std::string& data, const std::string& mime_type)
{
    return "data:" + mime_type + ";base64," +
            CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
            .ToString();
}

BluHandler::BluHandler()
    : m_is_closing(false)
{
    DCHECK(!g_instance);
    g_instance = this;
}

BluHandler::~BluHandler()
{
    g_instance = nullptr;
}

// static
BluHandler* BluHandler::GetInstance()
{
    return g_instance;
}

void BluHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Add to the list of existing browsers.
    m_browser_list.push_back(browser);
}

bool BluHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
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

void BluHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
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

void BluHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             ErrorCode errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl)
{
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

void BluHandler::CloseAllBrowsers(bool force_close)
{
    if (m_browser_list.empty())
        return;

    if (!CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::BindOnce(&BluHandler::CloseAllBrowsers, this,
                                           force_close));
        return;
    }

    BrowserList::const_iterator it = m_browser_list.begin();
    for (; it != m_browser_list.end(); ++it)
    {
        (*it)->GetHost()->CloseBrowser(force_close);
    }
}
