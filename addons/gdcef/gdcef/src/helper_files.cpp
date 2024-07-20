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

#include "helper_files.hpp"
#include <iostream>
#include <fstream>

#if defined(_WIN32)
#  include <Windows.h>
#else
#  include <unistd.h>
#endif

//------------------------------------------------------------------------------
bool are_valid_files(fs::path const& folder,
                     std::vector<std::string> const& files)
{
    bool failure = false;

    for (auto const& it: files)
    {
        fs::path f = { folder / it };
        // TODO Compute SHA1 on files to check if they are correct
        if (!fs::exists(f))
        {
            std::cout << "[GDCEF] [" << __func__ << "] File "
                      << f << " does not exist and is needed for CEF"
                      << std::endl;
            failure = true;
        }
    }

    return !failure;
}

//------------------------------------------------------------------------------
// Posible alternative (but /proc/self/exe will return the canoncial path even
// from an alias.
// extern char *__progname;
// return __progname;
std::string executable_name()
{
#if defined(_WIN32)
    // Pragma required for linking + windows.h
    #pragma comment(lib, "kernel32.lib")
    //const DWORD MAX_PATH = 64u; // KO - MAX_PATH already defined anyway
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return buf;

#else

    char path[1024];
    if (readlink("/proc/self/exe", path, 1024) == -1)
        return {};
    return path;

#endif
}

//------------------------------------------------------------------------------
fs::path real_path()
{
#if defined(_WIN32)

    // Step 1: Get the current path and concat your application executable name.
    //
    // Step 2: Get the canonical path of your application (ie the real path). This allows
    // to remove possible symlink.
    //
    // Step 3: Return the path without your application name
    return fs::canonical({ fs::current_path() / executable_name() }).parent_path();

#else //if defined(PLATFORM_POSIX) || defined(__linux__)

    // Since /proc/self/exe return the canoncial we can return it directly
    fs::path p(executable_name());
    return p.parent_path();

#endif
}
