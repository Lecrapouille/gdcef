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

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
#  define CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_

#  include "cef_includes.hpp"

class GDCefBrowser : public CefApp,
                     public CefBrowserProcessHandler,
                     public CefRenderProcessHandler
{
public:

    // CefApp methods:
    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
    {
        return this;
    }

    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
    {
        return this;
    }

    // CefBrowserProcessHandler methods:
    virtual void OnContextInitialized() override;

private:

    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) override;

    IMPLEMENT_REFCOUNTING(GDCefBrowser);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
