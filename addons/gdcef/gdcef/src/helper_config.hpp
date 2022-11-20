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
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//*****************************************************************************

#ifndef STIGMEE_GDCEF_HELPER_CONFIG_HPP
#  define STIGMEE_GDCEF_HELPER_CONFIG_HPP

#  include "gdcef.hpp"

// ****************************************************************************
//! \brief  CEF can be run either from the binary (standalone application) or
//! from the Godot editor. We have to distinguish the both case.
// ****************************************************************************
#define isStartedFromGodotEditor()                                            \
   godot::OS::get_singleton()->has_feature("editor")

#define globalize_path(path)                                                  \
   godot::ProjectSettings::get_singleton()->globalize_path(path.c_str()).utf8().get_data()

// ****************************************************************************
//! \brief Godot dictionary getter with default value.
// ****************************************************************************
template<class T>
static T getConfig(godot::Dictionary const& config, const char* property,
                   T const& default_value)
{
    if ((property != nullptr) && config.has(property))
        return config[property];
    return default_value;
}

template<>
std::string getConfig<std::string>(godot::Dictionary const& config,
                                   const char* property,
                                   std::string const& default_value)
{
    if ((property != nullptr) && config.has(property))
    {
        godot::String str = config[property];
        return str.utf8().get_data();
    }
    return default_value;
}

template<>
fs::path getConfig<fs::path >(godot::Dictionary const& config,
                              const char* property,
                              fs::path const& default_value)
{
    if ((property != nullptr) && config.has(property))
    {
        godot::String str = config[property];
        return str.utf8().get_data();
    }
    return default_value;
}

template<>
cef_state_t getConfig<cef_state_t>(godot::Dictionary const& config,
                                   const char* property,
                                   cef_state_t const& default_value)
{
    if ((property != nullptr) && config.has(property))
        return config[property] ? STATE_ENABLED : STATE_DISABLED;
    return default_value;
}

#endif // STIGMEE_GDCEF_HELPER_CONFIG_HPP
