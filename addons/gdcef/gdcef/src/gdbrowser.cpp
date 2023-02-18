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

//------------------------------------------------------------------------------
#include "gdbrowser.hpp"
#include "helper_files.hpp"

//------------------------------------------------------------------------------
// in a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method CefRefPtr<CefBrowser> m_browser;this is used to expose various methods of this class to Godot
void GDBrowserView::_register_methods()
{
    GDCEF_DEBUG();

    godot::register_method("close", &GDBrowserView::close);
    godot::register_method("id", &GDBrowserView::id);
    godot::register_method("get_error", &GDBrowserView::getError);
    godot::register_method("is_valid", &GDBrowserView::isValid);
    godot::register_method("get_texture", &GDBrowserView::texture);
    godot::register_method("set_zoom_level", &GDBrowserView::setZoomLevel);
    godot::register_method("load_url", &GDBrowserView::loadURL);
    godot::register_method("is_loaded", &GDBrowserView::loaded);
    godot::register_method("get_url", &GDBrowserView::getURL);
    godot::register_method("stop_loading", &GDBrowserView::stopLoading);
    godot::register_method("execute_javascript", &GDBrowserView::executeJavaScript);
    godot::register_method("has_previous_page", &GDBrowserView::canNavigateBackward);
    godot::register_method("has_next_page", &GDBrowserView::canNavigateForward);
    godot::register_method("previous_page", &GDBrowserView::navigateBackward);
    godot::register_method("next_page", &GDBrowserView::navigateForward);
    godot::register_method("resize", &GDBrowserView::reshape);
    godot::register_method("set_viewport", &GDBrowserView::viewport);
    godot::register_method("on_key_pressed", &GDBrowserView::keyPress);
    godot::register_method("on_mouse_moved", &GDBrowserView::mouseMove);
    godot::register_method("on_mouse_left_click", &GDBrowserView::leftClick);
    godot::register_method("on_mouse_right_click", &GDBrowserView::rightClick);
    godot::register_method("on_mouse_middle_click", &GDBrowserView::middleClick);
    godot::register_method("on_mouse_left_down", &GDBrowserView::leftMouseDown);
    godot::register_method("on_mouse_left_up", &GDBrowserView::leftMouseUp);
    godot::register_method("on_mouse_right_down", &GDBrowserView::rightMouseDown);
    godot::register_method("on_mouse_right_up", &GDBrowserView::rightMouseUp);
    godot::register_method("on_mouse_middle_down", &GDBrowserView::middleMouseDown);
    godot::register_method("on_mouse_middle_up", &GDBrowserView::middleMouseUp);
    godot::register_method("on_mouse_wheel_vertical", &GDBrowserView::mouseWheelVertical);
    godot::register_method("on_mouse_wheel_horizontal", &GDBrowserView::mouseWheelHorizontal);
    godot::register_method("set_muted", &GDBrowserView::mute);
    godot::register_method("is_muted", &GDBrowserView::muted);

    godot::register_signal<GDBrowserView>("page_loaded", "node", GODOT_VARIANT_TYPE_OBJECT);
}

//------------------------------------------------------------------------------
void GDBrowserView::_init()
{}

//------------------------------------------------------------------------------
godot::String GDBrowserView::getError()
{
    std::string err = m_error.str();
    m_error.clear();
    return {err.c_str()};
}

//------------------------------------------------------------------------------
int GDBrowserView::init(godot::String const& url, CefBrowserSettings const& settings,
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
GDBrowserView::GDBrowserView()
    : m_viewport({ 0.0f, 0.0f, 1.0f, 1.0f})
{
    BROWSER_DEBUG_VAL("Create Godot texture");

    m_impl = new GDBrowserView::Impl(*this);
    m_image.instance();
    m_texture.instance();
}

//------------------------------------------------------------------------------
GDBrowserView::~GDBrowserView()
{
    close();
}

//------------------------------------------------------------------------------
void GDBrowserView::getViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect)
{
    rect = CefRect(int(m_viewport[0] * m_width),
                   int(m_viewport[1] * m_height),
                   int(m_viewport[2] * m_width),
                   int(m_viewport[3] * m_height));
}

//------------------------------------------------------------------------------
// FIXME find a less naive algorithm and dirtyRects instead of the whole image
void GDBrowserView::onPaint(CefRefPtr<CefBrowser> /*browser*/,
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

    // Copy CEF image buffer to Godot PoolByteArray
    m_data.resize(TEXTURE_SIZE);
    godot::PoolByteArray::Write w = m_data.write();
    memcpy(&w[0], buffer, size_t(TEXTURE_SIZE));

    // Color conversion BGRA8 -> RGBA8: swap B and R chanels
    for (int i = 0; i < TEXTURE_SIZE; i += COLOR_CHANELS)
    {
        std::swap(w[i], w[i + 2]);
    }

    // Copy Godot PoolByteArray to Godot texture.
    m_image->create_from_data(width, height, false, godot::Image::FORMAT_RGBA8,
                              m_data);
    m_texture->create_from_image(m_image, godot::Texture::FLAG_VIDEO_SURFACE);
}

//------------------------------------------------------------------------------
void GDBrowserView::onLoadEnd(CefRefPtr<CefBrowser> /*browser*/,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
    // Emit signal only when top-level frame has succeeded.
    if ((httpStatusCode == 200) && (frame->IsMain()))
    {
        GDCEF_DEBUG_VAL("has ended loading " << frame->GetURL());

        // Emit signal for Godot script
        emit_signal("page_loaded", this);
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::setZoomLevel(double delta)
{
    BROWSER_DEBUG_VAL(delta);

    if (!m_browser)
        return;

    m_browser->GetHost()->SetZoomLevel(delta);
}

//------------------------------------------------------------------------------
void GDBrowserView::loadURL(godot::String url)
{
    BROWSER_DEBUG_VAL(url.utf8().get_data());

    m_browser->GetMainFrame()->LoadURL(url.utf8().get_data());
}

//------------------------------------------------------------------------------
bool GDBrowserView::loaded() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->HasDocument();
}

//------------------------------------------------------------------------------
godot::String GDBrowserView::getURL() const
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
void GDBrowserView::stopLoading()
{
    BROWSER_DEBUG();

    if (!m_browser)
        return;

    m_browser->StopLoad();
}

//------------------------------------------------------------------------------
void GDBrowserView::executeJavaScript(godot::String javascript)
{

    if (m_browser && m_browser->GetMainFrame())
    {
        CefString codeStr;
        codeStr.FromString(javascript.utf8().get_data());
        CefString urlStr;
        m_browser->GetMainFrame()->ExecuteJavaScript(codeStr, urlStr, 0);
    }
    else
    {
        BROWSER_ERROR("execute_javascript failed");
    }
}

//------------------------------------------------------------------------------
bool GDBrowserView::canNavigateBackward() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->CanGoBack();
}

//------------------------------------------------------------------------------
void GDBrowserView::navigateBackward()
{
    BROWSER_DEBUG();

    if ((m_browser != nullptr) && (m_browser->CanGoBack()))
    {
        m_browser->GoBack();
    }
}

//------------------------------------------------------------------------------
bool GDBrowserView::canNavigateForward() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->CanGoForward();
}

//------------------------------------------------------------------------------
void GDBrowserView::navigateForward()
{
    BROWSER_DEBUG();

    if ((m_browser != nullptr) && (m_browser->CanGoForward()))
    {
        m_browser->GoForward();
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::reshape(int w, int h)
{
    BROWSER_DEBUG_VAL(w << " x " << h);

    m_width = float(w);
    m_height = float(h);

    if (!m_browser || !m_browser->GetHost())
        return;

    m_browser->GetHost()->WasResized();
}

//------------------------------------------------------------------------------
bool GDBrowserView::viewport(float x, float y, float w, float h)
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
bool GDBrowserView::isValid() const
{
    BROWSER_DEBUG();

    if (!m_browser)
        return false;

    return m_browser->IsValid();
}

//------------------------------------------------------------------------------
void GDBrowserView::close()
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

//------------------------------------------------------------------------------
bool GDBrowserView::mute(bool mute)
{
    CEF_REQUIRE_UI_THREAD();
    if (m_browser == nullptr)
        return true;

    m_browser->GetHost()->SetAudioMuted(mute);
    return m_browser->GetHost()->IsAudioMuted();
}

//------------------------------------------------------------------------------
bool GDBrowserView::muted()
{
    CEF_REQUIRE_UI_THREAD();
    if (m_browser == nullptr)
        return true;

    return m_browser->GetHost()->IsAudioMuted();
}
