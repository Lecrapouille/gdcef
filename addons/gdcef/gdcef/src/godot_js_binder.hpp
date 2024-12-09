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

// Godot 4
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/variant.hpp>

// Chromium Embedded Framework
#include "cef_v8.h"

// -----------------------------------------------------------------------------
//! \brief Convert a Godot variant to a V8 value
// -----------------------------------------------------------------------------
CefRefPtr<CefV8Value> GodotToV8(const godot::Variant& godot_value);

// -----------------------------------------------------------------------------
//! \brief Convert a V8 value to a Godot variant
// -----------------------------------------------------------------------------
godot::Variant V8ToGodot(CefRefPtr<CefV8Value> v8_value);

// -----------------------------------------------------------------------------
//! \brief Convert a Godot variant to a CefValue
// -----------------------------------------------------------------------------
CefRefPtr<CefValue> GodotToCefVal(const godot::Variant& var);

// ****************************************************************************
//! \class GodotMethodInvoker
//! \brief Class to handle binding between JavaScript and GDScript methods
// ****************************************************************************
class GodotMethodInvoker: public CefV8Handler
{
public:

    GodotMethodInvoker(godot::Object* obj, const godot::StringName& method)
        : m_godot_object(obj), m_method_name(method)
    {
    }

    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override;

    IMPLEMENT_REFCOUNTING(GodotMethodInvoker);

private:

    godot::Object* m_godot_object;
    godot::StringName m_method_name;
};

#endif // GDCEF_GODOT_JS_BINDER_HPP