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

//------------------------------------------------------------------------------
bool GodotMethodHandler::Execute(const CefString& name,
                                 CefRefPtr<CefV8Value> object,
                                 const CefV8ValueList& arguments,
                                 CefRefPtr<CefV8Value>& retval,
                                 CefString& exception)
{
    DEBUG_RENDER_PROCESS(name.ToString());

    // Function does not exist.
    if (name != "callGodotMethod")
    {
        exception = "Function does not exist";
        DEBUG_RENDER_PROCESS(exception.ToString());
        return false;
    }

    // No browser created, we cannot call the method.
    if (m_browser == nullptr)
    {
        exception = "Browser pointer at NULL";
        DEBUG_RENDER_PROCESS(exception.ToString());
        return true;
    }

    // Check that there is at least the method name as argument.
    if (arguments.size() < 1 || !arguments[0]->IsString())
    {
        exception = "First argument must be the method name";
        DEBUG_RENDER_PROCESS(exception.ToString());
        return false;
    }

    // Create and configure the IPC message to the main process.
    CefRefPtr<CefProcessMessage> msg =
        CefProcessMessage::Create("callGodotMethod");
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

    // Store the context and frame for later use
    m_context = context;
    m_frame = frame;

    // No handler yet, we need to create it first
    m_handler = new GodotMethodHandler(browser);

    // Create global JavaScript objects and bind methods
    CefRefPtr<CefV8Value> global = context->GetGlobal();

    // Create a global Godot bridge object
    CefRefPtr<CefV8Value> godotBridge =
        CefV8Value::CreateObject(nullptr, nullptr);

    // Bind only the base callGodotMethod
    godotBridge->SetValue(
        "callGodotMethod",
        CefV8Value::CreateFunction("callGodotMethod", m_handler),
        V8_PROPERTY_ATTRIBUTE_NONE);

    // Define the godot object
    global->SetValue("godot", godotBridge, V8_PROPERTY_ATTRIBUTE_NONE);

    // 1. Setup JS -> Godot communication
    const char* jsToGodotSetup = R"(
        // Setup Godot proxy for method calls
        const rawGodot = godot;
        window.godot = new Proxy({}, {
            get: function(target, prop) {
                if (prop === 'callGodotMethod') {
                    return rawGodot.callGodotMethod;
                }
                return function(...args) {
                    return rawGodot.callGodotMethod(prop, ...args);
                };
            }
        });
    )";

    frame->ExecuteJavaScript(jsToGodotSetup, frame->GetURL(), 0);
    DEBUG_RENDER_PROCESS("JS -> Godot communication initialized");

    // 2. Setup Godot -> JS communication
    const char* godotToJsSetup = R"(
        // Setup Event System for receiving Godot events
        window.godotEventSystem = {
            listeners: new Map(),

            // Register a listener for an event.
            // The listener is a callback function that will be called when the event is emitted.
            // It takes two arguments: the event name and the data.
            // - The event name is the name of the event to listen for.
            // - The callback is the function to call when the event is emitted.
            on: function(eventName, callback) {
                if (!this.listeners.has(eventName)) {
                    this.listeners.set(eventName, new Set());
                }
                this.listeners.get(eventName).add(callback);
                console.log(`[GodotEventSystem] Registered listener for: ${eventName}`);
            },

            // Emit an event. It is called to signal that an event has occurred.
            // It executes all the callbacks associated with the event.
            // It takes two arguments: the event name and the data.
            // - The event name is the name of the event to emit.
            // - The data is the data to pass to the event listeners.
            emit: function(eventName, data) {
                console.log(`[GodotEventSystem] Emitting: ${eventName}`, data);
                if (!this.listeners.has(eventName)) {
                    console.warn(`[GodotEventSystem] No listeners for: ${eventName}`);
                    return;
                }

                this.listeners.get(eventName).forEach(callback => {
                    try {
                        callback(data);
                    } catch (error) {
                        console.error(`[GodotEventSystem] Error in listener for ${eventName}:`, error);
                    }
                });
            }
        };

        // Helper function to register event listeners
        window.registerGodotEvent = function(eventName, callback) {
            window.godotEventSystem.on(eventName, callback);
        };
    )";

    frame->ExecuteJavaScript(godotToJsSetup, frame->GetURL(), 0);
    DEBUG_RENDER_PROCESS("Godot -> JS communication initialized");
}

//------------------------------------------------------------------------------
std::string RenderProcess::ConvertCefValueToJS(CefRefPtr<CefValue> value)
{
    switch (value->GetType())
    {
        case VTYPE_DICTIONARY: {
            CefRefPtr<CefDictionaryValue> dict = value->GetDictionary();
            std::string result = "{";
            bool first = true;

            CefDictionaryValue::KeyList keys;
            dict->GetKeys(keys);
            for (const auto& key : keys)
            {
                if (!first)
                    result += ",";
                result += "'" + key.ToString() + "':";
                result += ConvertCefValueToJS(dict->GetValue(key));
                first = false;
            }
            return result + "}";
        }

        case VTYPE_LIST: {
            CefRefPtr<CefListValue> list = value->GetList();
            std::string result = "[";
            for (size_t i = 0; i < list->GetSize(); ++i)
            {
                if (i > 0)
                    result += ",";
                result += ConvertCefValueToJS(list->GetValue(i));
            }
            return result + "]";
        }

        case VTYPE_STRING:
            return "'" + value->GetString().ToString() + "'";

        case VTYPE_INT:
            return std::to_string(value->GetInt());

        case VTYPE_DOUBLE:
            return std::to_string(value->GetDouble());

        case VTYPE_BOOL:
            return value->GetBool() ? "true" : "false";

        case VTYPE_NULL:
            return "null";

        default:
            DEBUG_RENDER_PROCESS(
                "Unsupported type in conversion: " << value->GetType());
            return "null";
    }
}

//------------------------------------------------------------------------------
bool RenderProcess::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
    DEBUG_RENDER_PROCESS("Received message: " << message->GetName().ToString());

    if (message->GetName() == "GodotToJS")
    {
        // Get message arguments
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        if (args->GetSize() < 2)
        {
            DEBUG_RENDER_PROCESS("Invalid message format");
            return false;
        }

        // Get event name and data
        CefString eventName = args->GetString(0);
        CefRefPtr<CefValue> data = args->GetValue(1);

        // Convert data to JS
        std::string jsData = ConvertCefValueToJS(data);

        DEBUG_RENDER_PROCESS("Event: " << eventName.ToString()
                                       << " Data: " << jsData);

        // Create JavaScript to emit the event
        std::string jsCode =
            "if (window.godotEventSystem) { "
            "window.godotEventSystem.emit('" +
            eventName.ToString() + "', " + jsData +
            "); "
            "} else { console.error('godotEventSystem not found'); }";

        // Execute in the browser context
        frame->ExecuteJavaScript(jsCode, frame->GetURL(), 0);
        return true;
    }
    return false;
}