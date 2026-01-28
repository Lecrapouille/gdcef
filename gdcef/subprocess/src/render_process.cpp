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
#include "cef_parser.h" // For CefBase64Encode and CefBase64Decode

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
        ss << "\033[32m[Secondary Process][gdCEFBrowser::" << __func__ << "] " \
           << txt << "\033[0m";                                                \
        std::cout << ss.str() << std::endl;                                    \
    }

//------------------------------------------------------------------------------
bool GodotMethodHandler::Execute(const CefString& name,
                                 CefRefPtr<CefV8Value> /*object*/,
                                 const CefV8ValueList& arguments,
                                 CefRefPtr<CefV8Value>& retval,
                                 CefString& exception)
{
    DEBUG_RENDER_PROCESS(name.ToString());

    // No browser created, we cannot call the method.
    if (m_browser == nullptr)
    {
        exception = "Browser pointer at NULL";
        DEBUG_RENDER_PROCESS(exception.ToString());
        return true;
    }

    // Function does not exist.
    if (name != "callGodotMethod")
    {
        exception = "Function does not exist";
        DEBUG_RENDER_PROCESS(exception.ToString());
        return false;
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

    // Convert remaining arguments to JSON string
    std::string json_args = "[";
    for (size_t i = 1; i < arguments.size(); ++i)
    {
        if (i > 1)
            json_args += ",";
        json_args += V8ToJSON(arguments[i]);
    }
    json_args += "]";

    // Add JSON string as second argument
    args->SetString(1, json_args);

    // Send the message to the main process
    m_browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
    retval = CefV8Value::CreateBool(true);

    return true;
}

//------------------------------------------------------------------------------
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
        try {
            window.godotMethods = new Proxy({}, {
                get: function(target, prop) {
                    if (prop === 'callGodotMethod') {
                        return godot.callGodotMethod;
                    }
                    return function(...args) {
                        return godot.callGodotMethod(prop, ...args);
                    };
                }
            });
            console.log('[gdCEF] Communication bridge initialized');
        } catch (e) {
            console.error('[gdCEF] Error setting up communication bridge:', e);
        }
    )";

    frame->ExecuteJavaScript(jsToGodotSetup, frame->GetURL(), 0);
    DEBUG_RENDER_PROCESS("JS -> Godot communication initialized");

    // 2. Setup Godot -> JS communication
    const char* godotToJsSetup = R"(
        // Setup Event System for receiving Godot events
        try {
            // Reset or create the event system
            window.godotEvents = window.godotEvents || {
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
                window.godotEvents.on(eventName, callback);
            };

            console.log('[gdCEF] Event system initialized');
        } catch (e) {
            console.error('[gdCEF] Error setting up event system:', e);
        }
    )";

    frame->ExecuteJavaScript(godotToJsSetup, frame->GetURL(), 0);
    DEBUG_RENDER_PROCESS("Godot -> JS communication initialized");
}

//------------------------------------------------------------------------------
void RenderProcess::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefV8Context> context)
{
    DEBUG_RENDER_PROCESS(browser->GetIdentifier());

    // If the released context is the one we stored, we clean it
    if (m_context && m_context->IsSame(context))
    {
        // Execute a script to clean rawGodot before releasing the context
        const char* cleanup = R"(
            try {
                // Clean references
                window.godotMethods = undefined;
                window.godotEvents = undefined;
                window.registerGodotEvent = undefined;
                console.log('[gdCEF] Cleanup completed');
            } catch (e) {
                console.error('[gdCEF] Cleanup error:', e);
            }
        )";

        // Execute the cleanup script
        frame->ExecuteJavaScript(cleanup, frame->GetURL(), 0);

        // Reset our references
        m_context = nullptr;
        m_frame = nullptr;
        m_handler = nullptr;

        DEBUG_RENDER_PROCESS("Context released and cleaned up");
    }
}

//------------------------------------------------------------------------------
bool RenderProcess::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> /*browser*/,
    CefRefPtr<CefFrame> frame,
    CefProcessId /*source_process*/,
    CefRefPtr<CefProcessMessage> message)
{
    DEBUG_RENDER_PROCESS(
        "Received IPC message: " << message->GetName().ToString());

    if (message->GetName() == "godotEvents.emit")
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
        CefString jsonData = args->GetString(1);

        DEBUG_RENDER_PROCESS("Event: " << eventName.ToString()
                                       << " Data: " << jsonData.ToString());

        // Create JavaScript to emit the event
        std::string jsCode =
            "if (window.godotEvents) { "
            "window.godotEvents.emit('" +
            eventName.ToString() + "', " + jsonData.ToString() +
            "); "
            "} else { console.error('godotEvents not found'); }";

        // Execute in the browser context
        frame->ExecuteJavaScript(jsCode, frame->GetURL(), 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
std::string GodotMethodHandler::V8ToJSON(CefRefPtr<CefV8Value> value)
{
    if (value->IsNull() || value->IsUndefined())
    {
        return "null";
    }
    else if (value->IsBool())
    {
        return value->GetBoolValue() ? "true" : "false";
    }
    else if (value->IsInt())
    {
        return std::to_string(value->GetIntValue());
    }
    else if (value->IsDouble())
    {
        return std::to_string(value->GetDoubleValue());
    }
    else if (value->IsString())
    {
        std::string str = value->GetStringValue();
        // Escape special characters
        std::string escaped;
        escaped.reserve(str.length() + 2);
        escaped += '"';
        for (char c : str)
        {
            switch (c)
            {
                case '"':
                    escaped += "\\\"";
                    break;
                case '\\':
                    escaped += "\\\\";
                    break;
                case '\b':
                    escaped += "\\b";
                    break;
                case '\f':
                    escaped += "\\f";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                default:
                    escaped += c;
                    break;
            }
        }
        escaped += '"';
        return escaped;
    }
    else if (value->IsArray())
    {
        std::string result = "[";
        for (int i = 0; i < value->GetArrayLength(); ++i)
        {
            if (i > 0)
                result += ",";
            result += V8ToJSON(value->GetValue(i));
        }
        result += "]";
        return result;
    }
    else if (value->IsArrayBuffer())
    {
        size_t length = value->GetArrayBufferByteLength();
        void* data = value->GetArrayBufferData();

        if (data && length > 0)
        {
            // Convert to base64
            std::string base64 = CefBase64Encode(data, length);

            return "{ \"type\": \"binary\", \"format\": \"base64\", \"data\": "
                   "\"" +
                   base64 + "\", \"size\": " + std::to_string(length) + " }";
        }
        return "{ \"type\": \"binary\", \"format\": \"base64\", \"data\": "
               "\"\", \"size\": 0 }";
    }
    else if (value->IsObject())
    {
        std::string result = "{";
        std::vector<CefString> keys;
        value->GetKeys(keys);
        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (i > 0)
                result += ",";
            result += V8ToJSON(CefV8Value::CreateString(keys[i]));
            result += ":";
            result += V8ToJSON(value->GetValue(keys[i]));
        }
        result += "}";
        return result;
    }
    return "null";
}