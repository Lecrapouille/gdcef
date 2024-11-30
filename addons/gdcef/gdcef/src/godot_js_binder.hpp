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

#ifndef GDCEF_GODOT_JS_BINDER_HPP
#define GDCEF_GODOT_JS_BINDER_HPP

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <include/cef_v8.h>
#include <unordered_map>

// Ajouter avant la classe GodotJSBinder:
namespace std {
template <>
struct hash<godot::String>
{
    size_t operator()(const godot::String& str) const
    {
        return str.hash();
    }
};
} // namespace std

// ****************************************************************************
//! \class GodotJSBinder
//! \brief Class to handle binding between JavaScript and GDScript variables
// ****************************************************************************
class GodotJSBinder: public godot::RefCounted
{
    GDCLASS(GodotJSBinder, godot::RefCounted);

protected:

    static void _bind_methods();

public:

    //! \brief Constructor
    GodotJSBinder();

    //! \brief Destructor
    virtual ~GodotJSBinder();

    //! \brief Bind a JavaScript variable to a GDScript variable
    bool bind_variable(const godot::String& js_name,
                       const godot::Variant& value);

    //! \brief Get a JavaScript variable value
    godot::Variant get_js_variable(const godot::String& js_name);

    //! \brief Execute a JavaScript script and return the result
    //! \param[in] script The JavaScript code to execute
    //! \return The result of the execution converted to a Godot Variant
    godot::Variant execute_js(const godot::String& script);

    //! \brief Get last error messages
    godot::String getError();

    //! \brief Bind a JavaScript function to a GDScript method
    bool bind_function(const godot::String& js_name,
                       godot::Object* target,
                       const godot::String& method_name);

protected:

    //! \brief Convert V8 value to Godot variant
    godot::Variant v8_to_godot(const CefRefPtr<CefV8Value>& value);

    //! \brief Convert Godot variant to V8 value
    CefRefPtr<CefV8Value> godot_to_v8(const godot::Variant& value);

private:

    //! \brief V8 context for JavaScript execution
    CefRefPtr<CefV8Context> m_context;

    //! \brief Hold last error messages
    mutable std::stringstream m_error;

    //! \brief Map to store JavaScript-GDScript variable bindings
    std::unordered_map<godot::String, godot::Variant> m_bindings;

    struct FunctionBinding
    {
        godot::Object* target;
        godot::String method_name;
    };

    std::unordered_map<godot::String, FunctionBinding> m_function_bindings;
};

#endif // GDCEF_GODOT_JS_BINDER_HPP