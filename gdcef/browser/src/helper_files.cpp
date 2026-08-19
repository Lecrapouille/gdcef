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

#include "helper_files.hpp"
#include "helper_log.hpp"
#include <fstream>
#include <iostream>

#if defined(_WIN32)
#    include <Windows.h>
#else
#    include <unistd.h>
#    include <dlfcn.h>
#endif

//------------------------------------------------------------------------------
fs::path utf8_to_path(std::string const& utf8)
{
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.data()),
                                  utf8.size()));
}

//------------------------------------------------------------------------------
std::string path_to_utf8(fs::path const& path)
{
    std::u8string utf8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

//------------------------------------------------------------------------------
bool are_valid_files(fs::path const& folder,
                     std::vector<std::string> const& files)
{
    bool failure = false;

    for (auto const& it : files)
    {
        fs::path f = {folder / it};
        // TODO Compute SHA1 on files to check if they are correct
        if (!fs::exists(f))
        {
            PRINT_ERROR( //
                "CEF artifact " << f << " is missing and is needed for CEF");
            failure = true;
        }
    }

    return !failure;
}

//------------------------------------------------------------------------------
// Possible alternative (but /proc/self/exe will return the canonical path even
// from an alias.
// extern char *__progname;
// return __progname;
std::string executable_name()
{
#if defined(_WIN32)
// Pragma required for linking + windows.h
#    pragma comment(lib, "kernel32.lib")
    // const DWORD MAX_PATH = 64u; // KO - MAX_PATH already defined anyway
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return buf;

#else

    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1)
        return {};
    path[len] = '\0';  // readlink doesn't null-terminate
    return path;

#endif
}

//------------------------------------------------------------------------------
fs::path real_path()
{
#if defined(_WIN32)

    // Step 1: Get the current path and concat your application executable name.
    //
    // Step 2: Get the canonical path of your application (ie the real path).
    // This allows to remove possible symlink.
    //
    // Step 3: Return the path without your application name
    try
    {
        return fs::canonical({fs::current_path() / executable_name()})
            .parent_path();
    }
    catch (const fs::filesystem_error& e)
    {
        PRINT_ERROR("fs::canonical failed: " << e.what());
        return fs::current_path();
    }

#else // if defined(PLATFORM_POSIX) || defined(__linux__)

    // Since /proc/self/exe return the canonical we can return it directly
    fs::path p(executable_name());
    return p.parent_path();

#endif
}

//------------------------------------------------------------------------------
godot::String convert_godot_url(godot::String const& url)
{
    // Not a Godot path
    if (!url.begins_with("res://") && !url.begins_with("user://"))
        return url.utf8().get_data();

    // Convert Godot path to system path
    godot::String local_path = GLOBALIZE_PATH(url);

    // Check if the file exists
    if (!fs::exists(local_path.utf8().get_data()))
        return {};

    // Build the file:// URL
    return "file://" + local_path;
}

//------------------------------------------------------------------------------
// Helper function used by get_module_directory() to get an address inside this
// module
static void dummy_function_for_address() {}

//------------------------------------------------------------------------------
fs::path get_module_directory()
{
#if defined(_WIN32)
    // Get a handle to the module containing this function
    HMODULE hModule = nullptr;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&dummy_function_for_address),
            &hModule))
    {
        char path[MAX_PATH];
        if (GetModuleFileNameA(hModule, path, MAX_PATH) > 0)
        {
            return fs::path(path).parent_path();
        }
    }
    // Fallback to current directory
    PRINT_ERROR("Failed to get module directory, using current directory");
    return fs::current_path();

#else // Linux / macOS
    Dl_info dl_info;
    if (dladdr(reinterpret_cast<void*>(&dummy_function_for_address), &dl_info) != 0)
    {
        if (dl_info.dli_fname != nullptr)
        {
            try
            {
                return fs::canonical(fs::path(dl_info.dli_fname)).parent_path();
            }
            catch (const fs::filesystem_error& e)
            {
                PRINT_ERROR("fs::canonical failed: " << e.what());
                // Fallback: return the path without canonicalization
                return fs::path(dl_info.dli_fname).parent_path();
            }
        }
    }
    // Fallback to current directory
    PRINT_ERROR("Failed to get module directory, using current directory");
    return fs::current_path();

#endif
}