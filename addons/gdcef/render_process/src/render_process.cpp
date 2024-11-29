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

#ifndef CALL_GODOT_METHOD
#    define CALL_GODOT_METHOD "callGodotMethod"
#endif

//------------------------------------------------------------------------------
bool GodotMethodHandler::Execute(const CefString& name,
                                 CefRefPtr<CefV8Value> object,
                                 const CefV8ValueList& arguments,
                                 CefRefPtr<CefV8Value>& retval,
                                 CefString& exception)
{
    // Function does not exist.
    if (name != CALL_GODOT_METHOD)
    {
        return false;
    }

    // No browser created, we cannot call the method.
    if (m_browser == nullptr)
    {
        exception = "Browser pointer at NULL";
        return true;
    }

    // Check that there is at least the method name as argument.
    if (arguments.size() < 1 || !arguments[0]->IsString())
    {
        exception = "First argument must be the method name";
        return false;
    }

    // Create and configure the IPC message to the main process.
    CefRefPtr<CefProcessMessage> msg =
        CefProcessMessage::Create(CALL_GODOT_METHOD);
    CefRefPtr<CefListValue> args = msg->GetArgumentList();

    // Add the method name as first argument.
    args->SetString(0, arguments[0]->GetStringValue());

    // Add the arguments directly from V8 types.
    for (size_t i = 1; i < arguments.size(); ++i)
    {
        auto arg = arguments[i];
        if (arg->IsBool())
        {
            args->SetBool(i, arg->GetBoolValue());
        }
        else if (arg->IsInt())
        {
            args->SetInt(i, arg->GetIntValue());
        }
        else if (arg->IsDouble())
        {
            args->SetDouble(i, arg->GetDoubleValue());
        }
        else if (arg->IsString())
        {
            args->SetString(i, arg->GetStringValue());
        }
        else
        {
            // For other types, convert them to string
            args->SetString(i, arg->GetStringValue());
        }
    }

    // Send the message to the main process
    m_browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
    retval = CefV8Value::CreateBool(true);

    return true;
}

//------------------------------------------------------------------------------
// TODO Faire OnContextReleased ?
void RenderProcess::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context)
{
    DEBUG_RENDER_PROCESS(browser->GetIdentifier());

    // No handler yet, we need to create it first
    m_handler = new GodotMethodHandler(browser);

    // Create global JavaScript objects and bind methods
    CefRefPtr<CefV8Value> global = context->GetGlobal();

    // Create a global Godot bridge object
    CefRefPtr<CefV8Value> godotBridge =
        CefV8Value::CreateObject(nullptr, nullptr);

    // Bind methods from Godot to JavaScript
    godotBridge->SetValue(
        CALL_GODOT_METHOD,
        CefV8Value::CreateFunction(CALL_GODOT_METHOD, m_handler),
        V8_PROPERTY_ATTRIBUTE_NONE);

    global->SetValue("godot", godotBridge, V8_PROPERTY_ATTRIBUTE_NONE);
}