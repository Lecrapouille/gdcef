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

#ifndef GDCEF_HELPER_FILES_HPP
#define GDCEF_HELPER_FILES_HPP

#include "helper_log.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <vector>

// ****************************************************************************
// C++20 filesystem utilities. The include depends on the version of g++ or
// clang++
// ****************************************************************************
#if __has_include(<filesystem>)
#    include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#    error "Missing the <filesystem> header."
#endif

// ****************************************************************************
//! \brief Globalize a Godot path (res:// or user://) to a std::string path
//! \param[in] path The Godot path to globalize
//! \return The globalized path as std::string
// ****************************************************************************
#define GLOBALIZE_PATH(path)                \
    godot::ProjectSettings::get_singleton() \
        ->globalize_path(path)              \
        .utf8()                             \
        .get_data()

// ****************************************************************************
//! \brief Get the name of the current application since we cannot directly
//! access to the command line (argv[0]).
// ****************************************************************************
std::string executable_name();

// ****************************************************************************
//! \brief Check if needed files for CEF are present and are valid in the given
//! folder.
//! \param[in] folder the path folder in which all CEF assets shall be present.
//! \param[in] files the list of needed files.
//! \return the presence of all needed files.
//! \retval true if all files are present and valid.
//! \retval false if some files are missing or invalid.
// ****************************************************************************
bool are_valid_files(std::filesystem::path const& folder,
                     std::vector<std::string> const& files);

// ****************************************************************************
//! \brief Return the canonical path of the executable
// ****************************************************************************
fs::path real_path();

// ****************************************************************************
//! \brief Convert a Godot URL (res:// or user://) to a file:// URL
//! \param[in] url The Godot URL to convert
//! \return The converted URL as file:// if it's a valid Godot path, empty
//! string if file not found,
//!         or the original URL if it's not a Godot path
// ****************************************************************************
godot::String convert_godot_url(godot::String const& url);

#endif // GDCEF_HELPER_FILES_HPP
