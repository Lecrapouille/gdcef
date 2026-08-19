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
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

//------------------------------------------------------------------------------
AdBlocker::AdBlocker()
{
    GDCEF_DEBUG("Initializing EasyList-compatible ad blocker");
    loadDefaultRules();
}

//------------------------------------------------------------------------------
void AdBlocker::enable(bool enable)
{
    m_enabled = enable;
    GDCEF_DEBUG("Ad blocker " << (enable ? "enabled" : "disabled"));
}

//------------------------------------------------------------------------------
void AdBlocker::clearRules()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_blocked_domains.clear();
    m_exception_domains.clear();
    m_blocked_patterns.clear();
    m_exception_patterns.clear();
}

//------------------------------------------------------------------------------
std::string AdBlocker::getStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream ss;
    ss << "AdBlocker stats: "
       << m_blocked_domains.size() << " blocked domains, "
       << m_exception_domains.size() << " exception domains, "
       << m_blocked_patterns.size() << " blocked patterns, "
       << m_exception_patterns.size() << " exception patterns";
    return ss.str();
}

//------------------------------------------------------------------------------
std::string AdBlocker::extractDomain(const std::string& url) const
{
    // Find the start of the domain (after ://)
    size_t start = url.find("://");
    if (start == std::string::npos)
    {
        start = 0;
    }
    else
    {
        start += 3;
    }

    // Find the end of the domain (before / or : or ?)
    size_t end = url.find_first_of("/:?#", start);
    if (end == std::string::npos)
    {
        end = url.length();
    }

    std::string domain = url.substr(start, end - start);

    // Convert to lowercase for case-insensitive matching
    std::transform(domain.begin(), domain.end(), domain.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return domain;
}

//------------------------------------------------------------------------------
bool AdBlocker::addRule(const std::string& rule)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return addRuleUnlocked(rule);
}

//------------------------------------------------------------------------------
bool AdBlocker::addRuleUnlocked(const std::string& rule)
{
    // Skip empty lines and comments
    if (rule.empty() || rule[0] == '!' || rule[0] == '[')
    {
        return false;
    }

    // Trim whitespace
    std::string trimmed = rule;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

    if (trimmed.empty())
    {
        return false;
    }

    // Check for exception rule (starts with @@)
    bool is_exception = false;
    if (trimmed.substr(0, 2) == "@@")
    {
        is_exception = true;
        trimmed = trimmed.substr(2);
    }

    // Skip element hiding rules (contain ##, #@#, #?#, etc.)
    if (trimmed.find("##") != std::string::npos ||
        trimmed.find("#@#") != std::string::npos ||
        trimmed.find("#?#") != std::string::npos)
    {
        return false;
    }

    // Skip rules with unsupported options for now (contain $)
    // We could parse these later for more advanced filtering
    size_t dollar_pos = trimmed.find('$');
    if (dollar_pos != std::string::npos)
    {
        // Remove options part for simple pattern matching
        trimmed = trimmed.substr(0, dollar_pos);
    }

    if (trimmed.empty())
    {
        return false;
    }

    // Domain rule: ||domain.com^
    if (trimmed.substr(0, 2) == "||")
    {
        std::string domain = trimmed.substr(2);

        // Remove trailing ^ or / or *
        size_t end = domain.find_first_of("^/*");
        if (end != std::string::npos)
        {
            domain = domain.substr(0, end);
        }

        // Convert to lowercase
        std::transform(domain.begin(), domain.end(), domain.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (!domain.empty())
        {
            if (is_exception)
            {
                m_exception_domains.insert(domain);
                GDCEF_DEBUG("Added exception domain: " << domain);
            }
            else
            {
                m_blocked_domains.insert(domain);
                GDCEF_DEBUG("Added blocked domain: " << domain);
            }
            return true;
        }
    }
    // Path pattern rule: /path/pattern or |http://
    else if (trimmed[0] == '/' ||
             (trimmed[0] == '|' && trimmed.size() > 1 && trimmed[1] != '|'))
    {
        std::string pattern = trimmed;
        if (pattern[0] == '|')
        {
            pattern = pattern.substr(1);
        }

        // Convert to lowercase for case-insensitive matching
        std::transform(pattern.begin(), pattern.end(), pattern.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (!pattern.empty())
        {
            if (is_exception)
            {
                m_exception_patterns.push_back(pattern);
                GDCEF_DEBUG("Added exception pattern: " << pattern);
            }
            else
            {
                m_blocked_patterns.push_back(pattern);
                GDCEF_DEBUG("Added blocked pattern: " << pattern);
            }
            return true;
        }
    }
    // Simple domain or keyword (no special prefix)
    else if (!trimmed.empty() && trimmed[0] != '*')
    {
        // Treat as a substring pattern
        std::string pattern = trimmed;
        std::transform(pattern.begin(), pattern.end(), pattern.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (is_exception)
        {
            m_exception_patterns.push_back(pattern);
        }
        else
        {
            m_blocked_patterns.push_back(pattern);
        }
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
size_t AdBlocker::loadFilterList(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        PRINT_ERROR("Failed to open filter list: " << filepath);
        return 0;
    }

    size_t count = 0;
    std::string line;
    std::lock_guard<std::mutex> lock(m_mutex);
    while (std::getline(file, line))
    {
        if (addRuleUnlocked(line))
        {
            count++;
        }
    }

    GDCEF_DEBUG("Loaded " << count << " rules from " << filepath);
    return count;
}

//------------------------------------------------------------------------------
bool AdBlocker::isDomainBlocked(const std::string& domain) const
{
    // Check exact domain match
    if (m_blocked_domains.count(domain) > 0)
    {
        return true;
    }

    // Check parent domains (e.g., ads.example.com matches example.com rule)
    size_t pos = domain.find('.');
    while (pos != std::string::npos)
    {
        std::string parent = domain.substr(pos + 1);
        if (m_blocked_domains.count(parent) > 0)
        {
            return true;
        }
        pos = parent.find('.');
        if (pos != std::string::npos)
        {
            pos += (domain.length() - parent.length());
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool AdBlocker::isException(const std::string& url) const
{
    // Convert URL to lowercase for matching
    std::string lower_url = url;
    std::transform(lower_url.begin(), lower_url.end(), lower_url.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Check exception domains
    std::string domain = extractDomain(url);
    if (m_exception_domains.count(domain) > 0)
    {
        return true;
    }

    // Check parent domains for exceptions
    size_t pos = domain.find('.');
    while (pos != std::string::npos)
    {
        std::string parent = domain.substr(pos + 1);
        if (m_exception_domains.count(parent) > 0)
        {
            return true;
        }
        pos = parent.find('.');
        if (pos != std::string::npos)
        {
            pos += (domain.length() - parent.length());
        }
    }

    // Check exception patterns
    for (const auto& pattern : m_exception_patterns)
    {
        if (lower_url.find(pattern) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool AdBlocker::shouldBlock(const std::string& url) const
{
    // The rules can be edited from the Godot thread while we are matching them
    // from the CEF IO thread. isException() and isDomainBlocked() are called
    // with the lock held.
    std::lock_guard<std::mutex> lock(m_mutex);

    // First check if URL matches an exception
    if (isException(url))
    {
        return false;
    }

    // Convert URL to lowercase for matching
    std::string lower_url = url;
    std::transform(lower_url.begin(), lower_url.end(), lower_url.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Check domain blocking
    std::string domain = extractDomain(url);
    if (isDomainBlocked(domain))
    {
        return true;
    }

    // Check path patterns
    for (const auto& pattern : m_blocked_patterns)
    {
        if (lower_url.find(pattern) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
CefResourceRequestHandler::ReturnValue
AdBlocker::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefRequest> request,
                                CefRefPtr<CefCallback> callback)
{
    if (!m_enabled)
    {
        return RV_CONTINUE;
    }

    std::string url = request->GetURL().ToString();

    if (shouldBlock(url))
    {
        GDCEF_DEBUG("Blocked: " << url);
        return RV_CANCEL;
    }

    return RV_CONTINUE;
}

//------------------------------------------------------------------------------
void AdBlocker::loadDefaultRules()
{
    // =========================================================================
    // EasyList-style default rules
    // These are curated to avoid false positives while blocking common ads
    // =========================================================================

    // -------------------------------------------------------------------------
    // Major advertising networks (domains)
    // -------------------------------------------------------------------------
    const char* ad_domains[] = {
        // Google Ads
        "doubleclick.net",
        "googlesyndication.com",
        "googleadservices.com",
        "googletagservices.com",
        "googletagmanager.com",
        "pagead2.googlesyndication.com",
        "adservice.google.com",

        // Major ad networks
        "adnxs.com",
        "advertising.com",
        "adform.net",
        "adroll.com",
        "adsrvr.org",
        "amazon-adsystem.com",
        "bidswitch.net",
        "casalemedia.com",
        "criteo.com",
        "criteo.net",
        "demdex.net",
        "exoclick.com",
        "lijit.com",
        "liveintent.com",
        "liveramp.com",
        "mathtag.com",
        "media.net",
        "mediamath.com",
        "moatads.com",
        "mookie1.com",
        "openx.net",
        "outbrain.com",
        "pubmatic.com",
        "quantserve.com",
        "revcontent.com",
        "rfihub.com",
        "rlcdn.com",
        "rubiconproject.com",
        "scorecardresearch.com",
        "sharethis.com",
        "sharethrough.com",
        "simpli.fi",
        "smartadserver.com",
        "spotxchange.com",
        "taboola.com",
        "tapad.com",
        "teads.tv",
        "tremorhub.com",
        "tribalfusion.com",
        "turn.com",
        "undertone.com",
        "yieldmo.com",
        "zedo.com",

        // Social media trackers
        "connect.facebook.net",
        "pixel.facebook.com",
        "analytics.twitter.com",
        "ads-twitter.com",
        "platform.linkedin.com",
        "snap.licdn.com",

        // Analytics (major ones that are purely tracking)
        "hotjar.com",
        "mouseflow.com",
        "crazyegg.com",
        "clicktale.net",
        "fullstory.com",
        "luckyorange.com",
        "inspectlet.com",
        "logrocket.com",

        // Popup/redirect ads
        "popads.net",
        "popcash.net",
        "propellerads.com",
        "revcontent.com",
        "mgid.com",

        // Tracking pixels
        "pixel.wp.com",
        "pixel.quantserve.com",
        "bat.bing.com",
        "px.ads.linkedin.com",

        // Malvertising commonly used domains
        "adf.ly",
        "linkbucks.com",
        "shorte.st",
    };

    for (const auto& domain : ad_domains)
    {
        m_blocked_domains.insert(domain);
    }

    // -------------------------------------------------------------------------
    // Path patterns to block (careful selection to avoid false positives)
    // -------------------------------------------------------------------------
    const char* ad_patterns[] = {
        "/adserv",
        "/adsense",
        "/advert/",
        "/adserver/",
        "/admanager/",
        "/affiliate/click",
        "/banners/ads/",
        "/click-tracking/",
        "/clicktrack/",
        "/doubleclick/",
        "/native-ads/",
        "/pagead/",
        "/pop-under",
        "/popunder",
        "/pub/ads/",
        "/sponsored-content/",
        "/sponsored_content/",
        "/tracking-pixel",
        "/trackingpixel",
    };

    for (const auto& pattern : ad_patterns)
    {
        m_blocked_patterns.push_back(pattern);
    }

    // -------------------------------------------------------------------------
    // Exceptions (whitelist) - important legitimate resources
    // -------------------------------------------------------------------------
    const char* exception_patterns[] = {
        // Three.js and common JS libraries
        "threejs.org",
        "cdnjs.cloudflare.com",
        "unpkg.com",
        "jsdelivr.net",
        "npmcdn.com",

        // Google APIs (not ads)
        "apis.google.com",
        "fonts.googleapis.com",
        "fonts.gstatic.com",
        "maps.googleapis.com",
        "maps.google.com",
        "youtube.com",
        "googlevideo.com",
        "gstatic.com",

        // Common CDNs
        "akamaihd.net",
        "cloudflare.com",
        "fastly.net",
        "amazonaws.com",
        "azureedge.net",

        // GitHub
        "github.com",
        "githubusercontent.com",
        "github.io",
        "githubassets.com",

        // Common utility scripts (not tracking)
        "stats.js",
        "stats.module.js",
        "stats.min.js",
        "jquery",
        "react",
        "vue",
        "angular",
        "bootstrap",

        // Legitimate analytics that site owners need (opt-in)
        // Note: google-analytics is blocked by default, but can be whitelisted
    };

    for (const auto& pattern : exception_patterns)
    {
        // Domain-like patterns go to exception domains
        if (std::string(pattern).find('/') == std::string::npos &&
            std::string(pattern).find('.') != std::string::npos)
        {
            std::string domain = pattern;
            std::transform(domain.begin(), domain.end(), domain.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            m_exception_domains.insert(domain);
        }
        else
        {
            std::string pat = pattern;
            std::transform(pat.begin(), pat.end(), pat.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            m_exception_patterns.push_back(pat);
        }
    }

    GDCEF_DEBUG(getStats());
}
