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

#ifndef GDCEF_HELPER_LOG_HPP
#  define GDCEF_HELPER_LOG_HPP

#  include <iostream>
#  include <sstream>
#  include "godot_cpp/variant/utility_functions.hpp"

// ****************************************************************************
// Logging
// ****************************************************************************
#define GDCEF_DEBUG()                                                      \
do {                                                                       \
  std::stringstream ss;                                                    \
  ss << "[DEBUG][GDCEF][GDCEF::" << __func__ << "]" << std::endl;          \
  godot::UtilityFunctions::print_verbose(ss.str().c_str());                \
} while (0)

#define GDCEF_DEBUG_VAL(x)                                                 \
do {                                                                       \
  std::stringstream ss;                                                    \
  ss << "[DEBUG][GDCEF][GDCEF::" << __func__ << "] " << x << std::endl;    \
  godot::UtilityFunctions::print_verbose(ss.str().c_str());                \
} while (0)

#define GDCEF_ERROR(x)                                                     \
do {                                                                       \
  m_error.str(std::string());                                              \
  m_error << "[ERROR][GDCEF][GDCEF::" << __func__ << "] "                  \
          << x << std::endl;                                               \
  godot::UtilityFunctions::push_error(m_error.str().c_str());              \
} while (0)

#define GDCEF_WARNING(x)                                                   \
do {                                                                       \
  std::stringstream ss;                                                    \
  ss << "[WARNING][GDCEF][GDCEF::" << __func__ << "] " << x << std::endl;  \
  godot::UtilityFunctions::push_warning(ss.str().c_str());                 \
} while (0)

#define BROWSER_DEBUG()                                                    \
do {                                                                       \
  std::stringstream ss;                                                    \
  ss << "[DEBUG][GDCEF][BrowserView::" << __func__ << "][" << m_id << "]"  \
     << std::endl;                                                         \
  godot::UtilityFunctions::print_verbose(m_error.str().c_str());           \
} while (0)

#define BROWSER_DEBUG_VAL(x)                                               \
do {                                                                       \
  std::stringstream ss;                                                    \
  ss << "[DEBUG][GDCEF][BrowserView::" << __func__ << "]["                 \
     << m_id << "] " << x << std::endl;                                    \
  godot::UtilityFunctions::print_verbose(ss.str().c_str());                \
} while (0)

#define BROWSER_ERROR(x)                                                   \
do {                                                                       \
  m_error.str(std::string());                                              \
  m_error << "[ERROR][GDCEF][BrowserView::" << __func__ << "]["            \
          << m_id << "] " << x << std::endl;                               \
  godot::UtilityFunctions::push_error(m_error.str().c_str());              \
} while (0)

#define STATIC_GDCEF_ERROR(x)                                              \
do {                                                                       \
  std::stringstream ss;                                                    \
  ss << "[ERROR][GDCEF][" << __func__ << "]" << x << std::endl;            \
  godot::UtilityFunctions::push_error(ss.str().c_str());                   \
} while (0)

#endif // GDCEF_HELPER_LOG_HPP