// This code is a modification of the original projects that can be found at
// https://github.com/ashea-code/BluBrowser

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_

//#include "script_handler.h"
#include "include/cef_app.h"

class BluBrowser : public CefApp,
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

    // BluScriptHandler* handler;

    IMPLEMENT_REFCOUNTING(BluBrowser);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
