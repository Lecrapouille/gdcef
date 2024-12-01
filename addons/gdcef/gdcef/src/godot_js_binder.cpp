#include "godot_js_binder.hpp"
#include "helper_log.hpp"

//------------------------------------------------------------------------------
void GodotJSBinder::_bind_methods()
{
    using namespace godot;

    ClassDB::bind_method(D_METHOD("bind_variable", "js_name", "value"),
                         &GodotJSBinder::bind_variable);
    ClassDB::bind_method(D_METHOD("get_js_variable", "js_name"),
                         &GodotJSBinder::get_js_variable);
    ClassDB::bind_method(D_METHOD("get_error"), &GodotJSBinder::getError);
    ClassDB::bind_method(D_METHOD("execute_js", "script"),
                         &GodotJSBinder::execute_js);
    ClassDB::bind_method(
        D_METHOD("bind_function", "js_name", "target", "method_name"),
        &GodotJSBinder::bind_function);
    ClassDB::bind_method(D_METHOD("set_context_from", "browser"),
                         &GodotJSBinder::set_context_from);
}

//------------------------------------------------------------------------------
godot::String GodotJSBinder::getError()
{
    std::string err = m_error.str();
    m_error.clear();
    return {err.c_str()};
}

//------------------------------------------------------------------------------
bool GodotJSBinder::set_context_from(GDBrowserView* browser)
{
    JSBINDER_DEBUG();
    CEF_REQUIRE_UI_THREAD();

    if (browser == nullptr)
    {
        GDCEF_ERROR("Browser pointer is null");
        return false;
    }

    m_context = browser->getV8Context();
    if (m_context == nullptr)
    {
        GDCEF_ERROR("Failed to get V8 context");
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
CefRefPtr<CefV8Value> GodotJSBinder::godot_to_v8(const godot::Variant& value)
{
    JSBINDER_DEBUG();

    switch (value.get_type())
    {
        case godot::Variant::Type::NIL:
            return CefV8Value::CreateNull();

        case godot::Variant::Type::BOOL:
            return CefV8Value::CreateBool(value.operator bool());

        case godot::Variant::Type::INT:
            return CefV8Value::CreateInt(value.operator int64_t());

        case godot::Variant::Type::FLOAT:
            return CefV8Value::CreateDouble(value.operator double());

        case godot::Variant::Type::STRING:
            return CefV8Value::CreateString(
                value.operator godot::String().utf8().get_data());

        case godot::Variant::Type::ARRAY: {
            auto godot_array = value.operator godot::Array();
            auto v8_array = CefV8Value::CreateArray(godot_array.size());

            for (int i = 0; i < godot_array.size(); ++i)
            {
                v8_array->SetValue(i, godot_to_v8(godot_array[i]));
            }

            return v8_array;
        }

        case godot::Variant::Type::DICTIONARY: {
            auto godot_dict = value.operator godot::Dictionary();
            auto v8_object = CefV8Value::CreateObject(nullptr, nullptr);

            godot::Array keys = godot_dict.keys();
            for (int i = 0; i < keys.size(); ++i)
            {
                godot::Variant key = keys[i];
                godot::String key_str = key.operator godot::String();

                v8_object->SetValue(CefString(key_str.utf8().get_data()),
                                    godot_to_v8(godot_dict[key]),
                                    V8_PROPERTY_ATTRIBUTE_NONE);
            }

            return v8_object;
        }

        default:
            return CefV8Value::CreateNull();
    }
}

//------------------------------------------------------------------------------
godot::Variant GodotJSBinder::v8_to_godot(const CefRefPtr<CefV8Value>& v8_value)
{
    JSBINDER_DEBUG();

    if (!v8_value.get())
        return godot::Variant();

    if (v8_value->IsNull() || v8_value->IsUndefined())
        return godot::Variant();

    if (v8_value->IsBool())
        return godot::Variant(v8_value->GetBoolValue());

    if (v8_value->IsInt())
        return godot::Variant(v8_value->GetIntValue());

    if (v8_value->IsDouble())
        return godot::Variant(v8_value->GetDoubleValue());

    if (v8_value->IsString())
        return godot::Variant(
            godot::String(v8_value->GetStringValue().ToString().c_str()));

    if (v8_value->IsArray())
    {
        godot::Array godot_array;
        int length = v8_value->GetArrayLength();

        for (int i = 0; i < length; ++i)
        {
            godot_array.push_back(v8_to_godot(v8_value->GetValue(i)));
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
                    v8_to_godot(property);
            }
        }

        return godot_dict;
    }

    return godot::Variant();
}

//------------------------------------------------------------------------------
bool GodotJSBinder::bind_variable(const godot::String& js_name,
                                  const godot::Variant& value)
{
    JSBINDER_DEBUG();

    if (!m_context.get())
    {
        GDCEF_ERROR("No V8 context available");
        return false;
    }

    if (!m_context->Enter())
    {
        GDCEF_ERROR("Failed to enter V8 context");
        return false;
    }

    bool success = false;
    CefRefPtr<CefV8Value> v8_value = godot_to_v8(value);

    if (v8_value.get())
    {
        m_context->GetGlobal()->SetValue(
            js_name.utf8().get_data(), v8_value, V8_PROPERTY_ATTRIBUTE_NONE);
        success = true;
    }

    m_context->Exit();
    return success;
}

//------------------------------------------------------------------------------
godot::Variant GodotJSBinder::get_js_variable(const godot::String& js_name)
{
    JSBINDER_DEBUG();

    if (!m_context.get())
    {
        GDCEF_ERROR("No V8 context available");
        return godot::Variant();
    }

    if (!m_context->Enter())
    {
        GDCEF_ERROR("Failed to enter V8 context");
        return godot::Variant();
    }

    godot::Variant result;
    CefRefPtr<CefV8Value> value =
        m_context->GetGlobal()->GetValue(js_name.utf8().get_data());

    if (value.get())
    {
        result = v8_to_godot(value);
    }

    m_context->Exit();
    return result;
}

//------------------------------------------------------------------------------
godot::Variant GodotJSBinder::execute_js(const godot::String& script)
{
    JSBINDER_DEBUG();

    if (!m_context.get())
    {
        m_error << "No V8 context available";
        return godot::Variant();
    }

    if (!m_context->Enter())
    {
        m_error << "Failed to enter V8 context";
        return godot::Variant();
    }

    CefRefPtr<CefV8Value> retval;
    CefRefPtr<CefV8Exception> exception;

    CefString js_code(script.utf8().get_data());
    bool success = m_context->Eval(js_code, CefString(), 0, retval, exception);
    if (!success || exception.get())
    {
        if (exception.get())
        {
            m_error << "JavaScript error: "
                    << exception->GetMessage().ToString();
        }
        m_context->Exit();
        return godot::Variant();
    }

    godot::Variant result;
    if (retval.get())
    {
        result = v8_to_godot(retval);
    }

    m_context->Exit();
    return result;
}

//------------------------------------------------------------------------------
bool GodotJSBinder::bind_function(const godot::String& js_name,
                                  godot::Object* target,
                                  const godot::String& method_name)
{
    JSBINDER_DEBUG();

    if (!m_context.get() || !target)
    {
        GDCEF_ERROR("No V8 context available");
        return false;
    }

    if (!m_context->Enter())
    {
        GDCEF_ERROR("Failed to enter V8 context");
        return false;
    }

    // Créer une fonction JavaScript qui appellera la méthode GDScript
    std::string js_function = js_name.utf8().get_data();
    js_function += " = function(data) { window.gdscript_callback('";
    js_function += js_name.utf8().get_data();
    js_function += "', data); }";

    CefString js_code(js_function);

    CefRefPtr<CefV8Value> retval;
    CefRefPtr<CefV8Exception> exception;

    bool success = m_context->Eval(js_code, CefString(), 0, retval, exception);

    if (!success || exception.get())
    {
        if (exception.get())
        {
            m_error << "JavaScript error: "
                    << exception->GetMessage().ToString();
        }
        m_context->Exit();
        return false;
    }

    m_context->Exit();
    return success;
}