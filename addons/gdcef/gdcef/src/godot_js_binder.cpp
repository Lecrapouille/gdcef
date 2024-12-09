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

#include "godot_js_binder.hpp"

//------------------------------------------------------------------------------
bool GodotMethodInvoker::Execute(const CefString& name,
                                 CefRefPtr<CefV8Value> object,
                                 const CefV8ValueList& arguments,
                                 CefRefPtr<CefV8Value>& retval,
                                 CefString& exception)
{
    // Convert arguments JavaScript to Godot arguments
    godot::Array godot_args;
    for (const auto& arg : arguments)
    {
        godot_args.append(V8ToGodot(arg));
    }

    // Call the Godot method
    godot::Variant result = m_godot_object->call(m_method_name, godot_args);

    // Convert Godot result to V8 value
    retval = GodotToV8(result);
    return true;
}

//------------------------------------------------------------------------------
CefRefPtr<CefV8Value> GodotToV8(const godot::Variant& godot_value)
{
    switch (godot_value.get_type())
    {
        case godot::Variant::BOOL:
            return CefV8Value::CreateBool(godot_value.operator bool());

        case godot::Variant::INT:
            return CefV8Value::CreateInt(godot_value.operator int64_t());

        case godot::Variant::FLOAT:
            return CefV8Value::CreateDouble(godot_value.operator double());

        case godot::Variant::STRING:
            return CefV8Value::CreateString(CefString(
                godot_value.operator godot::String().utf8().get_data()));

        case godot::Variant::ARRAY: {
            godot::Array godot_array = godot_value.operator godot::Array();
            CefRefPtr<CefV8Value> js_array =
                CefV8Value::CreateArray(godot_array.size());

            for (int i = 0; i < godot_array.size(); ++i)
            {
                js_array->SetValue(i, GodotToV8(godot_array[i]));
            }
            return js_array;
        }

        case godot::Variant::DICTIONARY: {
            godot::Dictionary godot_dict =
                godot_value.operator godot::Dictionary();
            CefRefPtr<CefV8Value> js_object =
                CefV8Value::CreateObject(nullptr, nullptr);

            for (int i = 0; i < godot_dict.size(); ++i)
            {
                godot::Variant key = godot_dict.keys()[i];
                godot::Variant value = godot_dict.values()[i];

                js_object->SetValue(
                    key.operator godot::String().utf8().get_data(),
                    GodotToV8(value),
                    V8_PROPERTY_ATTRIBUTE_NONE);
            }
            return js_object;
        }

        default:
            return CefV8Value::CreateNull();
    }
}

//------------------------------------------------------------------------------
godot::Variant V8ToGodot(CefRefPtr<CefV8Value> v8_value)
{
    if (!v8_value.get())
    {
        return godot::Variant();
    }

    if (v8_value->IsNull() || v8_value->IsUndefined())
    {
        return godot::Variant();
    }

    if (v8_value->IsBool())
    {
        return godot::Variant(v8_value->GetBoolValue());
    }

    if (v8_value->IsInt())
    {
        return godot::Variant(v8_value->GetIntValue());
    }

    if (v8_value->IsDouble())
    {
        return godot::Variant(v8_value->GetDoubleValue());
    }

    if (v8_value->IsString())
    {
        return godot::Variant(
            godot::String(v8_value->GetStringValue().ToString().c_str()));
    }

    if (v8_value->IsArray())
    {
        godot::Array godot_array;
        int length = v8_value->GetArrayLength();

        for (int i = 0; i < length; ++i)
        {
            godot_array.append(V8ToGodot(v8_value->GetValue(i)));
        }

        return godot_array;
    }

    if (v8_value->IsObject())
    {
        godot::Dictionary godot_dict;
        std::vector<CefString> keys;
        v8_value->GetKeys(keys);

        for (const auto& key : keys)
        {
            CefRefPtr<CefV8Value> property = v8_value->GetValue(key);
            if (property.get())
            {
                godot_dict[godot::String(key.ToString().c_str())] =
                    V8ToGodot(property);
            }
        }

        return godot_dict;
    }

    return godot::Variant();
}

//------------------------------------------------------------------------------
CefRefPtr<CefValue> GodotToCefVal(const godot::Variant& var)
{
    CefRefPtr<CefValue> value = CefValue::Create();

    switch (var.get_type())
    {
        case godot::Variant::NIL:
            value->SetNull();
            break;

        case godot::Variant::BOOL:
            value->SetBool(static_cast<bool>(var));
            break;

        case godot::Variant::INT:
            value->SetInt(static_cast<int32_t>(var));
            break;

        case godot::Variant::FLOAT:
            value->SetDouble(static_cast<double>(var));
            break;

        case godot::Variant::STRING:
            value->SetString(static_cast<godot::String>(var).utf8().get_data());
            break;

        case godot::Variant::ARRAY: {
            godot::Array arr = var;
            CefRefPtr<CefListValue> list = CefListValue::Create();
            for (int i = 0; i < arr.size(); ++i)
            {
                CefRefPtr<CefValue> element = GodotToCefVal(arr[i]);
                list->SetValue(i, element);
            }
            value->SetList(list);
            break;
        }

        case godot::Variant::DICTIONARY: {
            godot::Dictionary dict = var;
            CefRefPtr<CefDictionaryValue> cef_dict =
                CefDictionaryValue::Create();
            godot::Array keys = dict.keys();
            for (int i = 0; i < keys.size(); ++i)
            {
                godot::String key = keys[i];
                CefRefPtr<CefValue> val = GodotToCefVal(dict[key]);
                cef_dict->SetValue(key.utf8().get_data(), val);
            }
            value->SetDictionary(cef_dict);
            break;
        }

        default:
            // Pour les types non gérés, on convertit en string
            value->SetString(var.stringify().utf8().get_data());
            break;
    }

    return value;
}