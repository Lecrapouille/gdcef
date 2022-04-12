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

//------------------------------------------------------------------------------
#include "browser.hpp"
#include "helper.hpp"

//------------------------------------------------------------------------------
// in a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method CefRefPtr<CefBrowser> m_browser;this is used to expose various methods of this class to Godot
void BrowserView::_register_methods()
{
    GDCEF_DEBUG();

    godot::register_method("close", &BrowserView::close);
    godot::register_method("id", &BrowserView::id);
    godot::register_method("is_valid", &BrowserView::isValid);
    godot::register_method("get_texture", &BrowserView::texture);
    godot::register_method("use_texture_from", &BrowserView::texture);
    godot::register_method("set_zoom_level", &BrowserView::setZoomLevel);
    godot::register_method("load_url", &BrowserView::loadURL);
    godot::register_method("is_loaded", &BrowserView::loaded);
    godot::register_method("get_url", &BrowserView::getURL);
    godot::register_method("stop_loading", &BrowserView::stopLoading);
    godot::register_method("has_previous_page", &BrowserView::canNavigateBackward);
    godot::register_method("has_next_page", &BrowserView::canNavigateForward);
    godot::register_method("previous_page", &BrowserView::navigateBackward);
    godot::register_method("next_page", &BrowserView::navigateForward);
    godot::register_method("resize", &BrowserView::reshape);
    godot::register_method("set_viewport", &BrowserView::viewport);
    godot::register_method("on_key_pressed", &BrowserView::keyPress);
    godot::register_method("on_mouse_moved", &BrowserView::mouseMove);
    godot::register_method("on_mouse_left_click", &BrowserView::leftClick);
    godot::register_method("on_mouse_right_click", &BrowserView::rightClick);
    godot::register_method("on_mouse_middle_click", &BrowserView::middleClick);
    godot::register_method("on_mouse_left_down", &BrowserView::leftMouseDown);
    godot::register_method("on_mouse_left_up", &BrowserView::leftMouseUp);
    godot::register_method("on_mouse_right_down", &BrowserView::rightMouseDown);
    godot::register_method("on_mouse_right_up", &BrowserView::rightMouseUp);
    godot::register_method("on_mouse_middle_down", &BrowserView::middleMouseDown);
    godot::register_method("on_mouse_middle_up", &BrowserView::middleMouseUp);
    godot::register_method("on_mouse_wheel", &BrowserView::mouseWheel);

    godot::register_signal<BrowserView>("page_loaded", "node", GODOT_VARIANT_TYPE_OBJECT);
}

//------------------------------------------------------------------------------
void BrowserView::_init()
{}

//------------------------------------------------------------------------------
int BrowserView::init(godot::String const& url, CefBrowserSettings const& settings,
                      CefWindowInfo const& window_info, godot::String const& name)
{
    // Create a new browser using the window parameters specified by
    // |windowInfo|.  If |request_context| is empty the global request context
    // will be used. This method can only be called on the browser process UI
    // thread. The optional |extra_info| parameter provides an opportunity to
    // specify extra information specific to the created browser that will be
    // passed to CefRenderProcessHandler::OnBrowserCreated() in the render
    // process.
    m_browser = CefBrowserHost::CreateBrowserSync(
        window_info, m_impl, url.utf8().get_data(), settings,
        nullptr, nullptr);

    if ((m_browser == nullptr) || (m_browser->GetHost() == nullptr))
    {
        m_id = -1;
        BROWSER_ERROR("CreateBrowserSync failed");
    }
    else
    {
        // Set Godot name
        set_name(name);

        m_id = m_browser->GetIdentifier();
        BROWSER_DEBUG_VAL("CreateBrowserSync #" << m_id << " "
                          << get_name().utf8().get_data()
                          << " succeeded");
        m_browser->GetHost()->WasResized();
    }

    return m_id;
}

//------------------------------------------------------------------------------
BrowserView::BrowserView()
    : m_viewport({ 0.0f, 0.0f, 1.0f, 1.0f})
{
    BROWSER_DEBUG_VAL("Create Godot texture");

    m_impl = new BrowserView::Impl(*this);
    m_image.instance();
    m_texture.instance();
}

//------------------------------------------------------------------------------
BrowserView::~BrowserView()
{
    close();
}

//------------------------------------------------------------------------------
void BrowserView::getViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect)
{
    rect = CefRect(int(m_viewport[0] * m_width),
                   int(m_viewport[1] * m_height),
                   int(m_viewport[2] * m_width),
                   int(m_viewport[3] * m_height));
}

//------------------------------------------------------------------------------
// FIXME find a less naive algorithm et utiliser dirtyRects
void BrowserView::onPaint(CefRefPtr<CefBrowser> /*browser*/,
                          CefRenderHandler::PaintElementType /*type*/,
                          const CefRenderHandler::RectList& /*dirtyRects*/,
                          const void* buffer, int width, int height)
{
    // Sanity check
    if ((width <= 0) || (height <= 0) || (buffer == nullptr))
        return ;

    // BGRA8: blue, green, red components each coded as byte
    int const COLOR_CHANELS = 4;
    int const SIZEOF_COLOR = COLOR_CHANELS * sizeof(char);
    int const TEXTURE_SIZE = SIZEOF_COLOR * width * height;

    // Copy CEF image buffer to Godot PoolVector
    m_data.resize(TEXTURE_SIZE);
    godot::PoolByteArray::Write w = m_data.write();
    memcpy(&w[0], buffer, size_t(TEXTURE_SIZE));

    // Color conversion BGRA8 -> RGBA8: swap B and R chanels
    for (int i = 0; i < TEXTURE_SIZE; i += COLOR_CHANELS)
    {
        std::swap(w[i], w[i + 2]);
    }

    // Copy Godot PoolVector to Godot texture.
    m_image->create_from_data(width, height, false, godot::Image::FORMAT_RGBA8, m_data);
    m_texture->create_from_image(m_image, godot::Texture::FLAG_VIDEO_SURFACE);
}

//------------------------------------------------------------------------------
void BrowserView::onLoadEnd(CefRefPtr<CefBrowser> /*browser*/,
                            CefRefPtr<CefFrame> /*frame*/,
                            int /*httpStatusCode*/)
{
    GDCEF_DEBUG_VAL("has ended loading");

    // Emit signal for Godot script
    emit_signal("page_loaded", this);
}

//------------------------------------------------------------------------------
void BrowserView::setZoomLevel(double delta)
{
    BROWSER_DEBUG_VAL(delta);

    if (!m_browser)
        return;

    m_browser->GetHost()->SetZoomLevel(delta);
}

//------------------------------------------------------------------------------
void BrowserView::loadURL(godot::String url)
{
    BROWSER_DEBUG_VAL(url.utf8().get_data());

    m_browser->GetMainFrame()->LoadURL(url.utf8().get_data());
}

//------------------------------------------------------------------------------
bool BrowserView::loaded() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->HasDocument();
}

//------------------------------------------------------------------------------
godot::String BrowserView::getURL() const
{
    if (m_browser && m_browser->GetMainFrame())
    {
        std::string str = m_browser->GetMainFrame()->GetURL().ToString();
        BROWSER_DEBUG_VAL(str);
        return str.c_str();
    }

    BROWSER_ERROR("Not possible to retrieving URL");
    return {};
}

//------------------------------------------------------------------------------
void BrowserView::stopLoading()
{
    BROWSER_DEBUG();

    if (!m_browser)
        return;

    m_browser->StopLoad();
}

//------------------------------------------------------------------------------
bool BrowserView::canNavigateBackward() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->CanGoBack();
}

//------------------------------------------------------------------------------
void BrowserView::navigateBackward()
{
    BROWSER_DEBUG();

    if ((m_browser != nullptr) && (m_browser->CanGoBack()))
    {
        m_browser->GoBack();
    }
}

//------------------------------------------------------------------------------
bool BrowserView::canNavigateForward() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->CanGoForward();
}

//------------------------------------------------------------------------------
void BrowserView::navigateForward()
{
    BROWSER_DEBUG();

    if ((m_browser != nullptr) && (m_browser->CanGoForward()))
    {
        m_browser->GoForward();
    }
}

//------------------------------------------------------------------------------
void BrowserView::reshape(int w, int h)
{
    BROWSER_DEBUG_VAL(w << " x " << h);

    m_width = float(w);
    m_height = float(h);

    if (!m_browser || !m_browser->GetHost())
        return;

    m_browser->GetHost()->WasResized();
}

//------------------------------------------------------------------------------
bool BrowserView::viewport(float x, float y, float w, float h)
{
    BROWSER_DEBUG_VAL(x << ", " << y << ", " << w << ", " << h);

    if (!(x >= 0.0f) && (x < 1.0f))
        return false;

    if (!(x >= 0.0f) && (y < 1.0f))
        return false;

    if (!(w > 0.0f) && (w <= 1.0f))
        return false;

    if (!(h > 0.0f) && (h <= 1.0f))
        return false;

    if (x + w > 1.0f)
        return false;

    if (y + h > 1.0f)
        return false;

    m_viewport[0] = x;
    m_viewport[1] = y;
    m_viewport[2] = w;
    m_viewport[3] = h;

    return true;
}

//------------------------------------------------------------------------------
bool BrowserView::isValid() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->IsValid();
}

//------------------------------------------------------------------------------
void BrowserView::close()
{
    BROWSER_DEBUG();

    if (!m_browser)
        return ;

    // FIXME
    BROWSER_DEBUG_VAL("'" << get_name().utf8().get_data() << "'");

    auto host = m_browser->GetHost();
    if (!host)
        return ;

    host->CloseDevTools(); // remote_debugging_port
    host->CloseBrowser(true); // TryCloseBrowser();
    m_browser = nullptr;
    m_impl = nullptr;
}
