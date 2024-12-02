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

#include "register_types.h"

#include "gdbrowser.hpp"
#include "gdcef.hpp"
#include "helper_log.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#ifdef __APPLE__
#    include "include/wrapper/cef_library_loader.h"
bool loaded = false;
#endif

using namespace godot;

void initialize_gdcef_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
        return;

    ClassDB::register_class<GDCef>();
    ClassDB::register_class<GDBrowserView>();

#ifdef __APPLE__
    String cef_artifacts_folder;
    if (OS::get_singleton()->has_feature("editor"))
        cef_artifacts_folder = ProjectSettings::get_singleton()->globalize_path(
            "res://" + String(CEF_ARTIFACTS_FOLDER));
    else
        cef_artifacts_folder =
            OS::get_singleton()->get_executable_path().get_base_dir().path_join(
                String(CEF_ARTIFACTS_FOLDER));

    // Load the CEF framework library.
    String framework_path = cef_artifacts_folder.path_join(
        "cefsimple.app/Contents/Frameworks/Chromium Embedded "
        "Framework.framework/Chromium Embedded Framework");
    std::string framework_path_std = std::string(framework_path.utf8());
    if (!cef_load_library(framework_path_std.c_str()))
    {
        GDCEF_DEBUG("Failed to load the CEF framework: " + framework_path_std);
        return;
    }
    GDCEF_DEBUG("Loaded the CEF framework: " + framework_path_std);
    loaded = true;
#endif
}

void uninitialize_gdcef_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
        return;

#ifdef __APPLE__
    if (loaded)
    {
        // Unload the CEF framework library.
        GDCEF_DEBUG("Unload the CEF framework.");
        cef_unload_library();
    }
#endif
}

extern "C" {

// Initialization.
GDExtensionBool GDE_EXPORT
gdcef_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                   const GDExtensionClassLibraryPtr p_library,
                   GDExtensionInitialization* r_initialization)
{
    godot::GDExtensionBinding::InitObject init_obj(
        p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_gdcef_module);
    init_obj.register_terminator(uninitialize_gdcef_module);
    init_obj.set_minimum_library_initialization_level(
        MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
