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

#ifndef AD_BLOCKER_HPP
#define AD_BLOCKER_HPP

#include "cef_resource_request_handler.h"

#include <regex>
#include <sstream>
#include <string>
#include <vector>

//! \brief Type definition for the ad blocker check function
//! \param[in] request_url URL of the request to check
//! \param[in] current_url Current URL of the page
//! \param[in] resource_type Type of resource (document, script, image)
//! \param[in] regexlist List of regex patterns to match against
//! \return 1 if the URL should be blocked, 0 otherwise
using UBlockOriginCheckFunction = int (*)(const char*,
                                          const char*,
                                          const char*,
                                          const char*);

//! \brief Ad blocker based on UBlock Origin DLL.
//!
//! This class provides an ad blocker based on the UBlock Origin DLL.
//! It allows to block ads by matching URLs against a list of regex patterns.
//! The patterns are read from the UBlock Origin repository.
//!
//! The ad blocker is implemented as a CEF resource request handler.
//! It is used to intercept resource requests and check if the URL should be
//! blocked.
//!
//! The ad blocker is initialized with default patterns and can be extended with
//! custom patterns.
class AdBlocker: public CefResourceRequestHandler
{
public:

    //! \brief Constructor loading default ad patterns
    AdBlocker();

    //! \brief Destructor
    ~AdBlocker();

    //! \brief Enable or disable the ad blocker
    inline void enable(bool enable)
    {
        m_enabled = enable;
    }

    //! \brief Check if the ad blocker is enabled
    inline bool isEnabled() const
    {
        return m_enabled;
    }

    //! \brief Add a file containing regex patterns
    //! \param[in] path Path to the file containing regex patterns
    //! \return true if the file is read successfully, false otherwise
    bool addRegexFile(const std::string& path);

    //! \brief Add a custom pattern to block
    //! \param[in] pattern Regex pattern to match URLs to block
    //! \return true if the pattern is valid, false otherwise
    bool addPattern(const std::string& pattern);

    //! \brief CEF callback to handle resource requests
    //! \return CEF_RESPONSE_FILTER_RESPONSE to block the request
    virtual CefResourceRequestHandler::ReturnValue
    OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefRequest> request,
                         CefRefPtr<CefCallback> callback) override;

    //! \brief CEF reference counting
    IMPLEMENT_REFCOUNTING(AdBlocker);

private:

    //! \brief Initialize the ad blocker from UBlock Origin DLL
    bool initUBlockOrigin();

    //! \brief Unload the UBlock Origin DLL
    void unloadUBlockOrigin();

    //! \brief Load the UBlock Origin pattern files
    bool loadUBlockOriginPatternFiles();

    //! \brief Read a file containing regex patterns.
    //! \param[in] file_path Path to the file containing regex patterns
    //! \return true if the file is read successfully, false otherwise
    bool readRegexFile(const std::string& file_path);

    //! \brief Initialize the ad blocker with default blacklist of URLs
    void initDefaultPatterns();

    //! \brief Add a regex pattern to block
    //! \param[in] pattern Regex pattern to match URLs to block
    //! \return true if the pattern is valid, false otherwise
    bool addDefaultPattern(const std::string& pattern);

private:

    //! \brief UBlock Origin DLL handler
    void* m_dll_handler = nullptr;

    //! \brief UBlock Origin DLL function to check if an URL should be blocked
    UBlockOriginCheckFunction m_function_adblocker = nullptr;

    //! \brief List of regex patterns to block
    std::stringstream m_regexlist;

    //! \brief Patterns to block when UBlock Origin is not available
    std::vector<std::regex> m_default_patterns;

    //! \brief Enable or disable the ad blocker
    bool m_enabled = true;
};

#endif
