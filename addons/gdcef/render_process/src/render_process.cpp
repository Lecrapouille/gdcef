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

//------------------------------------------------------------------------------
#define DEBUG_RENDER_PROCESS(txt)                                       \
    {                                                                   \
        std::stringstream ss;                                           \
        ss << "\033[32m[Secondary Process][RenderProcess::" << __func__ \
           << "] " << txt << "\033[0m";                                 \
        std::cout << ss.str() << std::endl;                             \
    }

//------------------------------------------------------------------------------
#define DEBUG_BROWSER_PROCESS(txt)                                             \
    {                                                                          \
        std::stringstream ss;                                                  \
        ss << "\033[32m[Secondary Process][GDCefBrowser::" << __func__ << "] " \
           << txt << "\033[0m";                                                \
        std::cout << ss.str() << std::endl;                                    \
    }

#define CALL_GODOT_METHOD "callGodotMethod"

//------------------------------------------------------------------------------
bool GodotMethodHandler::Execute(const CefString& name,
                                 CefRefPtr<CefV8Value> object,
                                 const CefV8ValueList& arguments,
                                 CefRefPtr<CefV8Value>& retval,
                                 CefString& exception)
{
    if (name == CALL_GODOT_METHOD)
    {
        // Convert JavaScript arguments to Godot types
        // Call corresponding Godot method
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
RenderProcess::~RenderProcess()
{
    DEBUG_RENDER_PROCESS("");
}

//------------------------------------------------------------------------------
void RenderProcess::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context)
{
    DEBUG_RENDER_PROCESS(browser->GetIdentifier());

    // Create global JavaScript objects and bind methods
    CefRefPtr<CefV8Value> global = context->GetGlobal();

    // Create a global Godot bridge object
    CefRefPtr<CefV8Value> godotBridge =
        CefV8Value::CreateObject(nullptr, nullptr);

    // Bind methods from Godot to JavaScript
    godotBridge->SetValue(
        CALL_GODOT_METHOD,
        CefV8Value::CreateFunction(CALL_GODOT_METHOD, new GodotMethodHandler()),
        V8_PROPERTY_ATTRIBUTE_NONE);

    global->SetValue("godot", godotBridge, V8_PROPERTY_ATTRIBUTE_NONE);
}