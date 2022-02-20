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

#include "gdcef_browser_app.hpp"
#include "gdcef_client.hpp"
#include <string>
#include <iostream>

void GDCefBrowser::OnContextInitialized()
{
    std::cout << "[SubProcess] [GDCefBrowser::OnContextInitialized] begin" << std::endl;
    CEF_REQUIRE_UI_THREAD();

    // Information used when creating the native window.
    CefWindowInfo window_info;

#if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    window_info.SetAsPopup(NULL, "BLUI");
#endif

    // GDCefClient implements browser-level callbacks.
    std::cout << "[SubProcess] [GDCefBrowser::OnContextInitialized] Create client handler" << std::endl;
    CefRefPtr<GDCefClient> handler(new GDCefClient());

    // Specify CEF browser settings here.
    CefBrowserSettings browser_settings;
    //FIXME browser_settings.file_access_from_file_urls = STATE_ENABLED;
    //FIXME browser_settings.universal_access_from_file_urls = STATE_ENABLED;

    // Check if a "--url=" value was provided via the command-line. If so, use
    // that instead of the default URL.
    std::cout << "[SubProcess] [GDCefBrowser::OnContextInitialized] Setting up command line" << std::endl;
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();
    std::string url(command_line->GetSwitchValue("url"));
    if (url.empty())
    {
        url = "https://labo.stigmee.fr";
    }

    // Create the first browser window.
    std::cout << "[SubProcess] [GDCefBrowser::OnContextInitialized] Create the browser" << std::endl;
    CefBrowserHost::CreateBrowser(window_info, handler.get(), url,
                                  browser_settings, nullptr, nullptr);

}

void GDCefBrowser::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context)
{
    std::cout << "[SubProcess] [GDCefBrowser::OnContextCreated]" << std::endl;
    // no handler yet, we need to create it first
    //FIXME handler = new BluScriptHandler(browser);

    // Retrieve the context's window object.
    CefRefPtr<CefV8Value> object = context->GetGlobal();

    //FIXME CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("blu_event", handler);

    // Add the string to the window object as "window.myval". See the "JS Objects" section below.
    //FIXME object->SetValue("blu_event", func, V8_PROPERTY_ATTRIBUTE_NONE);
}
