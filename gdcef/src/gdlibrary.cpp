#include "gdcef.h"
#include "browser.h"
//QQ #include "apphandler.h"

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options * o)
{
    godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options * o)
{
    godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT godot_nativescript_init(void* handle)
{
    godot::Godot::nativescript_init(handle);
    godot::register_class<GDCef>();
    godot::register_class<BrowserView>();
    //QQ godot::register_class<godot::AppHandler>();
}
