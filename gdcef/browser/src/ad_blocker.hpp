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
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

// =============================================================================
//! \brief Ad blocker based on Adblock Plus filter list format (EasyList compatible).
//!
//! Supports the following filter syntax:
//! - ||domain.com^ : Block requests to domain.com and subdomains
//! - @@||domain.com^ : Exception (whitelist) for domain.com
//! - /path/pattern : Block URLs containing this path pattern
//! - @@/path/pattern : Exception for path pattern
//! - domain.com : Simple domain blocking
//!
//! The blocker uses optimized matching:
//! - Hash-based domain lookup for O(1) domain matching
//! - Substring search for path patterns (no slow regex)
// =============================================================================
class AdBlocker : public CefResourceRequestHandler
{
public:

    // -------------------------------------------------------------------------
    //! \brief Constructor loading default EasyList-style rules.
    // -------------------------------------------------------------------------
    AdBlocker();

    // -------------------------------------------------------------------------
    //! \brief Enable or disable the ad blocker.
    //! \param[in] enable True to enable, false to disable.
    // -------------------------------------------------------------------------
    void enable(bool enable);

    // -------------------------------------------------------------------------
    //! \brief Check if the ad blocker is enabled.
    //! \return True if enabled, false otherwise.
    // -------------------------------------------------------------------------
    inline bool is_enabled() const
    {
        return m_enabled;
    }

    // -------------------------------------------------------------------------
    //! \brief Add an EasyList-style filter rule.
    //!
    //! Supported formats:
    //! - "||ads.example.com^" : Block domain and subdomains
    //! - "@@||example.com^" : Exception (whitelist)
    //! - "/ads/" : Block URLs containing /ads/
    //! - "@@/good-script.js" : Exception for specific path
    //!
    //! \param[in] rule The filter rule in EasyList format.
    //! \return True if the rule was parsed successfully.
    // -------------------------------------------------------------------------
    bool addRule(const std::string& rule);

    // -------------------------------------------------------------------------
    //! \brief Load filter rules from a file (EasyList format).
    //! \param[in] filepath Path to the filter list file.
    //! \return Number of rules successfully loaded.
    // -------------------------------------------------------------------------
    size_t loadFilterList(const std::string& filepath);

    // -------------------------------------------------------------------------
    //! \brief Clear all rules.
    // -------------------------------------------------------------------------
    void clearRules();

    // -------------------------------------------------------------------------
    //! \brief Get statistics about loaded rules.
    //! \return A string with rule counts.
    // -------------------------------------------------------------------------
    std::string getStats() const;

    // -------------------------------------------------------------------------
    //! \brief CEF callback to handle resource requests.
    //! \return RV_CANCEL to block the request, RV_CONTINUE to allow.
    // -------------------------------------------------------------------------
    virtual CefResourceRequestHandler::ReturnValue
    OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefRequest> request,
                         CefRefPtr<CefCallback> callback) override;

    //! \brief CEF reference counting.
    IMPLEMENT_REFCOUNTING(AdBlocker);

private:

    // -------------------------------------------------------------------------
    //! \brief Extract the domain from a URL.
    //! \param[in] url The full URL.
    //! \return The domain (e.g., "ads.example.com").
    // -------------------------------------------------------------------------
    std::string extractDomain(const std::string& url) const;

    // -------------------------------------------------------------------------
    //! \brief Check if a URL should be blocked.
    //! \param[in] url The URL to check.
    //! \return True if the URL should be blocked.
    // -------------------------------------------------------------------------
    bool shouldBlock(const std::string& url) const;

    // -------------------------------------------------------------------------
    //! \brief Check if a URL matches an exception rule.
    //! \param[in] url The URL to check.
    //! \return True if the URL is whitelisted.
    // -------------------------------------------------------------------------
    bool isException(const std::string& url) const;

    // -------------------------------------------------------------------------
    //! \brief Check if a domain matches any blocked domain (including parents).
    //! \param[in] domain The domain to check.
    //! \return True if blocked.
    // -------------------------------------------------------------------------
    bool isDomainBlocked(const std::string& domain) const;

    // -------------------------------------------------------------------------
    //! \brief Load default blocking rules.
    // -------------------------------------------------------------------------
    void loadDefaultRules();

    //! \brief Blocked domains (hash set for O(1) lookup).
    std::unordered_set<std::string> m_blocked_domains;

    //! \brief Exception domains (whitelist).
    std::unordered_set<std::string> m_exception_domains;

    //! \brief Path patterns to block (substring match).
    std::vector<std::string> m_blocked_patterns;

    //! \brief Path patterns to whitelist.
    std::vector<std::string> m_exception_patterns;

    //! \brief Enable flag.
    bool m_enabled = true;
};

#endif // AD_BLOCKER_HPP
