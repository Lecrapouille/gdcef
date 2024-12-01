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

#ifndef GDCEF_HELPER_LOG_HPP
#define GDCEF_HELPER_LOG_HPP

#include <iostream>
#include <sstream>

// ****************************************************************************
// Logging
// TODO disable log in release mode or if log is required (from Godot)
// TODO Use Godot print instead of cout/cerr
// ****************************************************************************
#define GDCEF_DEBUG() \
    std::cout << "[GDCEF][GDCEF::" << __func__ << "]" << std::endl

#define GDCEF_DEBUG_VAL(x) \
    std::cout << "[GDCEF][GDCEF::" << __func__ << "] " << x << std::endl

#define GDCEF_ERROR(x) \
    m_error << "[GDCEF][GDCEF::" << __func__ << "] " << x << std::endl

#define GDCEF_WARNING(x) \
    std::cout << "[GDCEF][GDCEF::" << __func__ << "] " << x << std::endl

#define BROWSER_DEBUG()                                                     \
    std::cout << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "]" \
              << std::endl

#define BROWSER_DEBUG_VAL(x)                                                 \
    std::cout << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "] " \
              << x << std::endl

#define BROWSER_ERROR(x)                                                   \
    m_error << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "] " \
            << x << std::endl;                                             \
    std::cerr << m_error.str()

#define STATIC_GDCEF_ERROR(x) \
    std::cerr << "[ERROR][GDCEF][" << __func__ << "]" << x << std::endl

#endif // GDCEF_HELPER_LOG_HPP