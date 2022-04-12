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

#ifndef STIGMEE_GDCEF_HELPER_HPP
#  define STIGMEE_GDCEF_HELPER_HPP

#  include <vector>
#  include <iostream>

// ****************************************************************************
// Logging
// ****************************************************************************
#define GDCEF_DEBUG()                                                      \
  std::cout << "[GDCEF][GDCEF::" << __func__ << "]" << std::endl
#define GDCEF_DEBUG_VAL(x)                                                 \
  std::cout << "[GDCEF][GDCEF::" << __func__ << "] " << x << std::endl
#define GDCEF_ERROR(x)                                                     \
  std::cerr << "[GDCEF][GDCEF::" << __func__ << "] " << x << std::endl
#define BROWSER_DEBUG()                                                    \
  std::cout << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "]"  \
            << std::endl
#define BROWSER_DEBUG_VAL(x)                                               \
  std::cout << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "] " \
            << x << std::endl
#define BROWSER_ERROR(x)                                                   \
  std::cerr << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "] " \
            << x << std::endl

// ****************************************************************************
// C++17 filesystem utilities. The include depends on the version of g++ or
// clang++ this C++17
// ****************************************************************************
#  if __has_include(<filesystem>)
#    include <filesystem>
namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#  else
#    error "Missing the <filesystem> header."
#  endif

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

#endif // STIGMEE_GDCEF_HELPER_HPP
