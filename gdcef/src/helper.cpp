//*************************************************************************
// Stigmee: The art to sanctuarize knowledge exchanges.
// Copyright 2021-2022 Alain Duron <duron.alain@gmail.com>
// Copyright 2021-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Stigmee.
//
// Stigmee is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//*************************************************************************

#include "helper.hpp"
#include <iostream>
#include <fstream>

#if defined(_WIN32)
#include <Windows.h>
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
            std::cout << "[GDCEF] [" << __func__ << " File "
                      << f << " does not exist and is needed for CEF"
                      << std::endl;
            failure = true;
        }
    }

    return !failure;
}

//------------------------------------------------------------------------------
std::string executable_name()
{
#if defined(PLATFORM_POSIX) || defined(__linux__)

    std::string sp;
    std::ifstream("/proc/self/comm") >> sp;
    return sp;

#elif defined(_WIN32)
    // Pragma required for linking + windows.h
    #pragma comment(lib, "kernel32.lib")
    //const DWORD MAX_PATH = 64u; // KO - MAX_PATH already defined anyway 
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return buf;

#else

    static_assert(false, "unrecognized platform");

#endif
}

//------------------------------------------------------------------------------
fs::path real_path()
{
    // Step 1: Get the current path and concat the Stigmee executable name.
    //
    // Step 2: Get the canonical path of Stigmee (ie the real path). This allows
    // to remove possible symlink.
    //
    // Step 3: Return the path without the Stigmee name
    return fs::canonical({ fs::current_path() / executable_name() }).parent_path();;
}
