//*****************************************************************************
// MIT License
//
// Copyright (c) 2024 Alain Duron <duron.alain@gmail.com>
// Copyright (c) 2024 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "ad_blocker.hpp"
#include "helper_log.hpp"
#include <array>
#include <fstream>

#if defined(_WIN32)
#    include <Windows.h> // For DLL loading on Windows
#else
#    include <dlfcn.h> // For dynamic loading on Linux/macOS
#endif

//------------------------------------------------------------------------------
AdBlocker::AdBlocker()
{
    GDCEF_DEBUG("");

    // Load UBlock Origin DLL
    if (initUBlockOrigin())
    {
        // Load files ofUBlock Origin patterns
        if (!loadUBlockOriginPatternFiles())
        {
            GDCEF_ERROR("Failed to load UBlock Origin patterns. I'll use "
                        "degraded method.");
            unloadUBlockOrigin();
            initDefaultPatterns();
        }
    }
    else
    {
        GDCEF_ERROR(
            "Failed to load UBlock Origin DLL. I'll use degraded method.");

        initDefaultPatterns();
    }
}

//------------------------------------------------------------------------------
AdBlocker::~AdBlocker()
{
    unloadUBlockOrigin();
}

//------------------------------------------------------------------------------
bool AdBlocker::initUBlockOrigin()
{
#if defined(__linux__)
    auto dll = CEF_ARTIFACTS_FOLDER "/adblocker_dll.so"; // Linux shared library
#elif __APPLE__
    auto dll =
        CEF_ARTIFACTS_FOLDER "/adblocker_dll.dylib"; // macOS shared library
#elif _WIN32
    auto dll = CEF_ARTIFACTS_FOLDER "/adblocker_dll.dll"; // Windows DLL
#endif

// Platform-specific library loading
#if defined(_WIN32)
    m_dll_handler = LoadLibrary(dll);
    if (!m_dll_handler)
    {
        GDCEF_ERROR("Failed to load DLL: " << dll);
        return false;
    }

    // Get the function pointer for the check function
    m_function_adblocker =
        (UBlockOriginCheckFunction)GetProcAddress(m_dll_handler, "check");
    if (!m_function_adblocker)
    {
        GDCEF_ERROR("Failed to find the 'check' function in the DLL.");
        return false;
    }
#else
    m_dll_handler = dlopen(dll, RTLD_NOW);
    if (!m_dll_handler)
    {
        GDCEF_ERROR("Failed to load shared library: " << dlerror());
        return false;
    }

    m_function_adblocker =
        (UBlockOriginCheckFunction)dlsym(m_dll_handler, "check");
    const char* error = dlerror();
    if (error)
    {
        GDCEF_ERROR(
            "Failed to find the 'check' function in the shared library: "
            << error);
        return false;
    }
#endif

    return true;
}

//------------------------------------------------------------------------------
void AdBlocker::unloadUBlockOrigin()
{
    if (m_dll_handler)
    {
#if defined(_WIN32)
        // Unload the DLL on Windows
        FreeLibrary(m_dll_handler);
#else
        // Unload the shared library on Linux/macOS
        dlclose(m_dll_handler);
#endif
    }
}

//------------------------------------------------------------------------------
bool AdBlocker::readRegexFile(const std::string& file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        GDCEF_ERROR("Failed to open file: " << file_path);
        return false;
    }

    m_regexlist << file.rdbuf() << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool AdBlocker::loadUBlockOriginPatternFiles()
{
    readRegexFile(CEF_ARTIFACTS_FOLDER
                  "/adblock-rust/data/easylist.to/easylist/easylist.txt");
    readRegexFile(CEF_ARTIFACTS_FOLDER
                  "/adblock-rust/data/easylist.to/easylist/easyprivacy.txt");
    readRegexFile(CEF_ARTIFACTS_FOLDER
                  "/adblock-rust/data/uBlockOrigin/badware.txt");
    readRegexFile(CEF_ARTIFACTS_FOLDER
                  "/adblock-rust/data/uBlockOrigin/filters.txt");
    readRegexFile(CEF_ARTIFACTS_FOLDER
                  "/adblock-rust/data/uBlockOrigin/resource-abuse.txt");

    if (m_regexlist.str().empty())
    {
        GDCEF_ERROR("Failed to load regex patterns from UBlock Origin.");
        return false;
    }
}

//------------------------------------------------------------------------------
void AdBlocker::initDefaultPatterns()
{
    const std::vector<std::string> default_patterns = {
        R"(.*doubleclick\.net.*)",
        R"(.*googlesyndication\.com.*)",
        R"(.*google-analytics\.com.*)",
        R"(.*adnxs\.com.*)",
        R"(.*advertising\.com.*)",
        R"(.*/ads/.*)",
        R"(.*/adserv.*)",
        R"(.*/banner.*)",
        R"(.*/analytics.*)",
        R"(.*/tracker.*)",
    };

    for (const auto& pattern : default_patterns)
    {
        addDefaultPattern(pattern);
    }
}

//------------------------------------------------------------------------------
bool AdBlocker::addDefaultPattern(const std::string& pattern)
{
    try
    {
        m_default_patterns.push_back(std::regex(pattern, std::regex::icase));
        return true;
    }
    catch (const std::regex_error& e)
    {
        GDCEF_ERROR("Invalid ad blocking pattern: " << pattern);
        return false;
    }
}

//------------------------------------------------------------------------------
bool AdBlocker::addPattern(const std::string& pattern)
{
    GDCEF_DEBUG("Adding ad block pattern " << pattern);

    // If UBlock Origin is available, use it
    if (m_function_adblocker != nullptr)
    {
        m_regexlist << pattern << std::endl;
        return true;
    }
    else // Use default patterns
    {
        return addDefaultPattern(pattern);
    }
}

//------------------------------------------------------------------------------
bool AdBlocker::addRegexFile(const std::string& path)
{
    // If UBlock Origin is available, use it
    if (m_function_adblocker != nullptr)
    {
        return readRegexFile(path);
    }
    else // Use default patterns
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            GDCEF_ERROR("Failed to open file: " << path);
            return false;
        }

        std::string line;
        while (std::getline(file, line))
        {
            // Skip empty lines and comments
            if (!line.empty() && line[0] != '#' && line[0] != '!')
            {
                addDefaultPattern(line);
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------
CefResourceRequestHandler::ReturnValue
AdBlocker::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefRequest> request,
                                CefRefPtr<CefCallback> callback)
{
    if (m_enabled)
    {
        std::string request_url = request->GetURL().ToString();
        std::string current_url = browser->GetMainFrame()->GetURL().ToString();

        // Call Ublock Origin DLL
        if (m_function_adblocker != nullptr)
        {
            // List of resource types to check
            const std::array<const char*, 3> resource_types = {
                "document", "script", "image"};

            // Check each resource type
            for (const auto& resource_type : resource_types)
            {
                if (m_function_adblocker(request_url.c_str(),
                                         current_url.c_str(),
                                         resource_type,
                                         m_regexlist.str().c_str()) == 1)
                {
                    GDCEF_DEBUG("Blocked ad URL: " << request_url << " (type: "
                                                   << resource_type << ")");
                    return RV_CANCEL; // Block the request
                }
            }
        }
        else // Use default patterns
        {
            for (const auto& pattern : m_default_patterns)
            {
                if (std::regex_match(request_url, pattern))
                {
                    GDCEF_DEBUG("Blocked ad URL: " << request_url);
                    return RV_CANCEL; // Block the request
                }
            }
        }
    }

    return RV_CONTINUE; // Allow the request
}