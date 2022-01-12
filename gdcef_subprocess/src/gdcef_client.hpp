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

#ifndef GDCEF_SUBPROCESS_CLIENT_HPP
#define GDCEF_SUBPROCESS_CLIENT_HPP

#include <cef_client.h>
#include <list>

class GDCefClient : public CefClient,
                    public CefDisplayHandler,
                    public CefLifeSpanHandler,
                    public CefLoadHandler
{
public:

    GDCefClient();
    ~GDCefClient();

    // Provide access to the single global instance of this object.
    static GDCefClient* GetInstance();

    // CefClient methods:
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
    {
        return this;
    }

    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
    {
        return this;
    }

    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override
    {
        return this;
    }

    // CefLifeSpanHandler methods:
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // CefLoadHandler methods:
    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             ErrorCode errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl) override;

    // Request that all existing browser windows close.
    void CloseAllBrowsers(bool force_close);

    bool IsClosing() const
    {
        return m_is_closing;
    }

private:

    // List of existing browser windows. Only accessed on the CEF UI thread.
    using BrowserList = std::list<CefRefPtr<CefBrowser>>;

    BrowserList m_browser_list;
    bool m_is_closing;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(GDCefClient);
};

#endif // GDCEF_SUBPROCESS_CLIENT_HPP
