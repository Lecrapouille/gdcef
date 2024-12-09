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

#include "gdbrowser.hpp"
#include "godot_js_binder.hpp"
#include "helper_config.hpp"
#include "helper_files.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#ifdef _OPENMP
#    include <omp.h>
#endif

//------------------------------------------------------------------------------
// Visit the html content of the current page.
class Visitor: public CefStringVisitor
{
public:

    Visitor(GDBrowserView& node) : m_node(node) {}

    virtual void Visit(const CefString& string) override
    {
        godot::String html(string.ToString().c_str());

        m_node.emit_signal("on_html_content_requested", html, &m_node);
    }

private:

    GDBrowserView& m_node;
    IMPLEMENT_REFCOUNTING(Visitor);
};

//------------------------------------------------------------------------------
GDBrowserView::Impl::~Impl()
{
    WARN_PRINT("[gdCEF][GDBrowserView::Impl::~Impl] destroying browser");
}

//------------------------------------------------------------------------------
// in a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method CefRefPtr<CefBrowser> m_browser; this is used to expose various
// methods of this class to Godot.
void GDBrowserView::_bind_methods()
{
    WARN_PRINT("[gdCEF][GDBrowserView::_bind_methods]");

    using namespace godot;

    // Methods
    ClassDB::bind_method(D_METHOD("close"), &GDBrowserView::close);
    ClassDB::bind_method(D_METHOD("id"), &GDBrowserView::id);
    ClassDB::bind_method(D_METHOD("get_error"), &GDBrowserView::getError);
    ClassDB::bind_method(D_METHOD("is_valid"), &GDBrowserView::isValid);
    ClassDB::bind_method(D_METHOD("set_texture", "texture"),
                         &GDBrowserView::setTexture);
    ClassDB::bind_method(D_METHOD("get_texture"), &GDBrowserView::getTexture);
    ClassDB::bind_method(D_METHOD("set_zoom_level"),
                         &GDBrowserView::setZoomLevel);
    ClassDB::bind_method(D_METHOD("get_title"), &GDBrowserView::getTitle);
    ClassDB::bind_method(D_METHOD("get_url"), &GDBrowserView::getURL);
    ClassDB::bind_method(D_METHOD("load_url"), &GDBrowserView::loadURL);
    ClassDB::bind_method(D_METHOD("load_data_uri"),
                         &GDBrowserView::loadDataURI);
    ClassDB::bind_method(D_METHOD("download_file"),
                         &GDBrowserView::downloadFile);
    ClassDB::bind_method(D_METHOD("allow_downloads"),
                         &GDBrowserView::allowDownloads);
    ClassDB::bind_method(D_METHOD("set_download_folder"),
                         &GDBrowserView::setDownloadFolder);
    ClassDB::bind_method(D_METHOD("is_loaded"), &GDBrowserView::loaded);
    ClassDB::bind_method(D_METHOD("reload"), &GDBrowserView::reload);
    ClassDB::bind_method(D_METHOD("stop_loading"), &GDBrowserView::stopLoading);
    ClassDB::bind_method(D_METHOD("copy"), &GDBrowserView::copy);
    ClassDB::bind_method(D_METHOD("paste"), &GDBrowserView::paste);
    ClassDB::bind_method(D_METHOD("undo"), &GDBrowserView::undo);
    ClassDB::bind_method(D_METHOD("redo"), &GDBrowserView::redo);
    ClassDB::bind_method(D_METHOD("request_html_content"),
                         &GDBrowserView::requestHtmlContent);
    ClassDB::bind_method(D_METHOD("execute_javascript"),
                         &GDBrowserView::executeJavaScript);
    ClassDB::bind_method(D_METHOD("has_previous_page"),
                         &GDBrowserView::canNavigateBackward);
    ClassDB::bind_method(D_METHOD("has_next_page"),
                         &GDBrowserView::canNavigateForward);
    ClassDB::bind_method(D_METHOD("previous_page"),
                         &GDBrowserView::navigateBackward);
    ClassDB::bind_method(D_METHOD("next_page"),
                         &GDBrowserView::navigateForward);
    ClassDB::bind_method(D_METHOD("resize"), &GDBrowserView::resize);
    ClassDB::bind_method(D_METHOD("set_viewport"), &GDBrowserView::viewport);
    ClassDB::bind_method(D_METHOD("set_key_pressed"), &GDBrowserView::keyPress);
    ClassDB::bind_method(D_METHOD("set_mouse_moved"),
                         &GDBrowserView::mouseMove);
    ClassDB::bind_method(D_METHOD("set_mouse_left_click"),
                         &GDBrowserView::leftClick);
    ClassDB::bind_method(D_METHOD("set_mouse_right_click"),
                         &GDBrowserView::rightClick);
    ClassDB::bind_method(D_METHOD("set_mouse_middle_click"),
                         &GDBrowserView::middleClick);
    ClassDB::bind_method(D_METHOD("set_mouse_left_down"),
                         &GDBrowserView::leftMouseDown);
    ClassDB::bind_method(D_METHOD("set_mouse_left_up"),
                         &GDBrowserView::leftMouseUp);
    ClassDB::bind_method(D_METHOD("set_mouse_right_down"),
                         &GDBrowserView::rightMouseDown);
    ClassDB::bind_method(D_METHOD("set_mouse_right_up"),
                         &GDBrowserView::rightMouseUp);
    ClassDB::bind_method(D_METHOD("set_mouse_middle_down"),
                         &GDBrowserView::middleMouseDown);
    ClassDB::bind_method(D_METHOD("set_mouse_middle_up"),
                         &GDBrowserView::middleMouseUp);
    ClassDB::bind_method(D_METHOD("set_mouse_wheel_vertical"),
                         &GDBrowserView::mouseWheelVertical);
    ClassDB::bind_method(D_METHOD("set_mouse_wheel_horizontal"),
                         &GDBrowserView::mouseWheelHorizontal);
    ClassDB::bind_method(D_METHOD("set_muted"), &GDBrowserView::mute);
    ClassDB::bind_method(D_METHOD("is_muted"), &GDBrowserView::muted);
    ClassDB::bind_method(D_METHOD("set_audio_stream", "audio"),
                         &GDBrowserView::setAudioStreamer);
    ClassDB::bind_method(D_METHOD("get_audio_stream"),
                         &GDBrowserView::getAudioStreamer);
    ClassDB::bind_method(D_METHOD("get_pixel_color", "x", "y"),
                         &GDBrowserView::getPixelColor);
    ClassDB::bind_method(D_METHOD("register_method"),
                         &GDBrowserView::registerGodotMethod);
    ClassDB::bind_method(D_METHOD("send_to_js", "event_name", "data"),
                         &GDBrowserView::sendToJS);

    // Signals
    ADD_SIGNAL(MethodInfo("on_download_updated",
                          PropertyInfo(Variant::STRING, "file"),
                          PropertyInfo(Variant::INT, "percentage"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(
        MethodInfo("on_page_loaded", PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_page_failed_loading",
                          PropertyInfo(Variant::INT, "err_code"),
                          PropertyInfo(Variant::STRING, "err_msg"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_browser_paint",
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_html_content_requested",
                          PropertyInfo(Variant::STRING, "html"),
                          PropertyInfo(Variant::OBJECT, "browser")));

    // Properties
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
                              "audio_stream",
                              PROPERTY_HINT_NODE_TYPE,
                              "AudioStreamGeneratorPlayback"),
                 "set_audio_stream",
                 "get_audio_stream");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
                              "texture",
                              PROPERTY_HINT_NODE_TYPE,
                              "ImageTexture"),
                 "set_texture",
                 "get_texture");
}

//------------------------------------------------------------------------------
void GDBrowserView::_init()
{
    BROWSER_DEBUG("");
}

//------------------------------------------------------------------------------
godot::String GDBrowserView::getError()
{
    std::string err = m_error.str();
    m_error.clear();
    return {err.c_str()};
}

//------------------------------------------------------------------------------
int GDBrowserView::init(godot::String const& url,
                        CefBrowserSettings const& settings,
                        CefWindowInfo const& window_info)
{
    // Create a new browser using the window parameters specified by
    // |windowInfo|.  If |request_context| is empty the global request context
    // will be used. This method can only be called on the browser process UI
    // thread. The optional |extra_info| parameter provides an opportunity to
    // specify extra information specific to the created browser that will be
    // passed to CefRenderProcessHandler::OnBrowserCreated() in the render
    // process.
    m_browser = CefBrowserHost::CreateBrowserSync(
        window_info, m_impl, url.utf8().get_data(), settings, nullptr, nullptr);

    if ((m_browser == nullptr) || (m_browser->GetHost() == nullptr))
    {
        m_id = -1;
        BROWSER_ERROR("CreateBrowserSync failed");
    }
    else
    {
        // Set Godot node default name
        std::string name("browser_");
        name += std::to_string(m_browser->GetIdentifier());
        set_name(name.c_str());

        m_id = m_browser->GetIdentifier();
        m_browser->GetHost()->WasResized();
    }

    return m_id;
}

//------------------------------------------------------------------------------
GDBrowserView::GDBrowserView() : m_viewport({0.0f, 0.0f, 1.0f, 1.0f})
{
    BROWSER_DEBUG("Creating new GDBrowserView");

    m_impl = new GDBrowserView::Impl(*this);
    assert((m_impl != nullptr) && "Failed allocating GDBrowserView");
    m_image.instantiate();
    m_texture.instantiate();
}

//------------------------------------------------------------------------------
GDBrowserView::~GDBrowserView()
{
    close();
}

//------------------------------------------------------------------------------
void GDBrowserView::getViewRect(CefRefPtr<CefBrowser> /*browser*/,
                                CefRect& rect)
{
    rect = CefRect(int(m_viewport[0] * m_width),
                   int(m_viewport[1] * m_height),
                   int(m_viewport[2] * m_width),
                   int(m_viewport[3] * m_height));
}

//------------------------------------------------------------------------------
void GDBrowserView::onPaint(CefRefPtr<CefBrowser> /*browser*/,
                            CefRenderHandler::PaintElementType /*type*/,
                            const CefRenderHandler::RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height)
{
    // Sanity check
    if ((width <= 0) || (height <= 0) || (buffer == nullptr))
        return;

    // BGRA8: blue, green, red components each coded as byte
    int const COLOR_CHANELS = 4;
    int const SIZEOF_COLOR = COLOR_CHANELS * sizeof(char);
    int const TEXTURE_SIZE = SIZEOF_COLOR * width * height;

    bool bResized = m_data.size() != TEXTURE_SIZE;

    // Copy CEF image buffer to Godot PoolByteArray
    m_data.resize(TEXTURE_SIZE);

    // Copy per line func for OpenMP/PPL
    unsigned char* imageData = m_data.ptrw();
    const unsigned char* cbuffer = (const unsigned char*)buffer;
    auto doCopyLine = [imageData, cbuffer, width, COLOR_CHANELS](
                          int line, int x, int copyWidth) {
        int i = (line * width + x) * COLOR_CHANELS;
        int end = i + (copyWidth * COLOR_CHANELS);
        for (; i < end; i += COLOR_CHANELS)
        {
            // Color conversion BGRA8 -> RGBA8: swap B and R chanels
            imageData[i + 0] = cbuffer[i + 2];
            imageData[i + 1] = cbuffer[i + 1];
            imageData[i + 2] = cbuffer[i + 0];
            imageData[i + 3] = cbuffer[i + 3];
        }
    };

    if (bResized)
    {
        // PPL
        // concurrency::parallel_for(0, height,
        //    std::bind(doCopyLine, std::placeholders::_1, 0, width));

#pragma omp parallel for
        for (int y = 0; y < height; ++y)
        {
            doCopyLine(y, 0, width);
        }

        // Copy Godot PoolByteArray to Godot texture.
        m_image->set_data(
            width, height, false, godot::Image::FORMAT_RGBA8, m_data);
        m_texture->set_image(m_image);
    }
    else
    {
        for (const CefRect& rect : dirtyRects)
        {
            // PPL
            // concurrency::parallel_for(rect.y, rect.y + rect.height,
            //     std::bind(doCopyLine, std::placeholders::_1, rect.x,
            //     rect.width));

#pragma omp parallel for
            for (int y = rect.y; y < rect.y + rect.height; ++y)
            {
                doCopyLine(y, rect.x, rect.width);
            }
        }

        // Copy Godot PoolByteArray to Godot texture.
        m_image->set_data(
            width, height, false, godot::Image::FORMAT_RGBA8, m_data);
        m_texture->update(m_image);
    }

    emit_signal("on_browser_paint", this);
}

//------------------------------------------------------------------------------
void GDBrowserView::onLoadEnd(CefRefPtr<CefBrowser> /*browser*/,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
    // Emit signal only when top-level frame has succeeded.
    if ((httpStatusCode == 200) && (frame->IsMain()))
    {
        BROWSER_DEBUG("has ended loading " << frame->GetURL());

        // Emit signal for Godot script
        emit_signal("on_page_loaded", this);
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::onLoadError(CefRefPtr<CefBrowser> /*browser*/,
                                CefRefPtr<CefFrame> frame,
                                const int errCode,
                                const CefString& errorText)
{
    CEF_REQUIRE_UI_THREAD();

    // Ignore download errors
    if (errCode == ERR_ABORTED)
        return;

    if (frame->IsMain())
    {
        std::string str = errorText.ToString();
        BROWSER_ERROR("has failed loading " << frame->GetURL() << ": " << str);
        godot::String msg(str.c_str());
        // Emit signal for Godot script
        emit_signal("on_page_failed_loading", errCode, msg, this);
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::setZoomLevel(double delta)
{
    BROWSER_DEBUG(delta);

    if (!m_browser)
        return;

    m_browser->GetHost()->SetZoomLevel(delta);
}

//------------------------------------------------------------------------------
void GDBrowserView::loadURL(godot::String url)
{
    godot::String converted_url = convert_godot_url(url);
    if (!converted_url.is_empty())
    {
        // Load the URL (either converted local file or original URL)
        BROWSER_DEBUG(converted_url.utf8().get_data());
        m_browser->GetMainFrame()->LoadURL(converted_url.utf8().get_data());
        return;
    }

    godot::String globalized_url = GLOBALIZE_PATH(url);
    BROWSER_ERROR("File not found: " << globalized_url.utf8().get_data());

    // Create error HTML page
    std::string error_html("<html><body bgcolor=\"white\">");
    error_html += "<h2>File not found: ";
    error_html += url.utf8().get_data();
    error_html += "</h2></body></html>";

    // Load the error page using data URI
    loadDataURI(godot::String(error_html.c_str()), "text/html");
}

//------------------------------------------------------------------------------
void GDBrowserView::loadDataURI(godot::String html, godot::String mime_type)
{
    auto const& d = html.utf8();
    std::string uri("data:");
    uri += mime_type.utf8().get_data();
    uri += ";base64,";
    uri += CefURIEncode(CefBase64Encode(d.ptr(), d.length()), false).ToString();
    m_browser->GetMainFrame()->LoadURL(uri);
}

//------------------------------------------------------------------------------
bool GDBrowserView::reload() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    m_browser->Reload();
    return true;
}

//------------------------------------------------------------------------------
bool GDBrowserView::loaded() const
{
    BROWSER_DEBUG("");

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
        BROWSER_DEBUG(str);
        return {str.c_str()};
    }

    BROWSER_ERROR("Not possible to retrieving URL");
    return {};
}

//------------------------------------------------------------------------------
godot::String GDBrowserView::getTitle() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return godot::String();

    if (m_browser->GetMainFrame())
    {
        CefString title =
            m_browser->GetHost()->GetVisibleNavigationEntry()->GetTitle();
        std::string utf8_title = title.ToString();
        return godot::String::utf8(utf8_title.c_str());
    }

    return godot::String();
}

//------------------------------------------------------------------------------
void GDBrowserView::stopLoading()
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return;

    m_browser->StopLoad();
}

//------------------------------------------------------------------------------
// FIXME https://github.com/chromiumembedded/cef/issues/3117
void GDBrowserView::copy() const
{
    BROWSER_DEBUG("");

    if (m_browser && m_browser->GetMainFrame())
    {
        m_browser->GetMainFrame()->Copy();
    }
    else
    {
        BROWSER_ERROR("copy failed");
    }
}

//------------------------------------------------------------------------------
// FIXME https://github.com/chromiumembedded/cef/issues/3117
void GDBrowserView::paste() const
{
    BROWSER_DEBUG("");

    if (m_browser && m_browser->GetMainFrame())
    {
        m_browser->GetMainFrame()->Paste();
    }
    else
    {
        BROWSER_ERROR("paste failed");
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::cut() const
{
    BROWSER_DEBUG("");

    if (m_browser && m_browser->GetMainFrame())
    {
        m_browser->GetMainFrame()->Cut();
    }
    else
    {
        BROWSER_ERROR("cut failed");
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::delete_() const
{
    BROWSER_DEBUG("");

    if (m_browser && m_browser->GetMainFrame())
    {
        m_browser->GetMainFrame()->Delete();
    }
    else
    {
        BROWSER_ERROR("delete failed");
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::undo() const
{
    BROWSER_DEBUG("");

    if (m_browser && m_browser->GetMainFrame())
    {
        m_browser->GetMainFrame()->Undo();
    }
    else
    {
        BROWSER_ERROR("undo failed");
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::redo() const
{
    BROWSER_DEBUG("");

    if (m_browser && m_browser->GetMainFrame())
    {
        m_browser->GetMainFrame()->Redo();
    }
    else
    {
        BROWSER_ERROR("redo failed");
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::requestHtmlContent()
{
    CefRefPtr<Visitor> visitor = new Visitor(*this);
    if (m_browser && m_browser->GetMainFrame())
    {
        m_browser->GetMainFrame()->GetSource(visitor);
    }

    BROWSER_ERROR("Not possible to retrieving text");
}

//------------------------------------------------------------------------------
void GDBrowserView::executeJavaScript(godot::String javascript)
{
    BROWSER_DEBUG("");

    if (m_browser && m_browser->GetMainFrame())
    {
        CefString codeStr;
        codeStr.FromString(javascript.utf8().get_data());
        CefString urlStr;
        m_browser->GetMainFrame()->ExecuteJavaScript(codeStr, urlStr, 0);
    }
    else
    {
        BROWSER_ERROR("executeJavaScript failed");
    }
}

//------------------------------------------------------------------------------
bool GDBrowserView::canNavigateBackward() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    return m_browser->CanGoBack();
}

//------------------------------------------------------------------------------
void GDBrowserView::navigateBackward()
{
    BROWSER_DEBUG("");

    if ((m_browser != nullptr) && (m_browser->CanGoBack()))
    {
        m_browser->GoBack();
    }
}

//------------------------------------------------------------------------------
bool GDBrowserView::canNavigateForward() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    return m_browser->CanGoForward();
}

//------------------------------------------------------------------------------
void GDBrowserView::navigateForward()
{
    BROWSER_DEBUG("");

    if ((m_browser != nullptr) && (m_browser->CanGoForward()))
    {
        m_browser->GoForward();
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::resize_(int width, int height)
{
    if (width <= 0)
    {
        width = 2;
    }
    if (height <= 0)
    {
        height = 2;
    }
    BROWSER_DEBUG(width << " x " << height);

    m_width = float(width);
    m_height = float(height);

    if (!m_browser || !m_browser->GetHost())
        return;

    m_browser->GetHost()->WasResized();
}

//------------------------------------------------------------------------------
bool GDBrowserView::viewport(float x, float y, float w, float h)
{
    // BROWSER_DEBUG(x << ", " << y << ", " << w << ", " << h);

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
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    return m_browser->IsValid();
}

//------------------------------------------------------------------------------
void GDBrowserView::close()
{
    if (!m_browser)
        return;

    godot::String name = get_name();
    BROWSER_DEBUG("Closing browser " << m_id << " '" << name.utf8().get_data()
                                     << "'");

    auto host = m_browser->GetHost();
    if (!host)
        return;

    host->CloseDevTools();    // remote_debugging_port
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

//------------------------------------------------------------------------------
void GDBrowserView::onAudioStreamStarted(CefRefPtr<CefBrowser> browser,
                                         const CefAudioParameters& params,
                                         int channels)
{
    m_impl->m_audio.channels = int(params.channel_layout);
}

//------------------------------------------------------------------------------
void GDBrowserView::onAudioStreamPacket(CefRefPtr<CefBrowser> browser,
                                        const float** data,
                                        int frames,
                                        int64_t pts)
{
    if ((m_impl == nullptr) || (m_impl->m_audio.streamer == nullptr))
    {
        return;
    }

    if ((data == nullptr) || (frames <= 0) || (m_impl->m_audio.channels == -1))
        return;

    auto& streamer = *(m_impl->m_audio.streamer.ptr());
    if (streamer.can_push_buffer(frames))
    {
        for (int i = 0; i < frames; i++)
        {
            streamer.push_frame(godot::Vector2(data[0][i], data[0][i]));
        }
    }
}

//------------------------------------------------------------------------------
godot::Color GDBrowserView::getPixelColor(int x, int y) const
{
    // Ensure the browser is valid and coordinates are within bounds
    if (x < 0 || y < 0 || x >= m_width || y >= m_height || m_data.size() == 0)
        return godot::Color(1, 1, 1, 1); // Return full white as fallback

    int index = (y * m_width + x) * 4;
    unsigned char r = m_data[index + 0];
    unsigned char g = m_data[index + 1];
    unsigned char b = m_data[index + 2];
    unsigned char a = m_data[index + 3];

    return godot::Color(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
}

//------------------------------------------------------------------------------
bool GDBrowserView::onBeforePopup(CefRefPtr<CefBrowser> browser,
                                  const CefString& target_url)
{
    // Prevent opening page on new windows.
    // See: https://github.com/Lecrapouille/gdcef/issues/19
    browser->GetMainFrame()->LoadURL(target_url);
    return true;
}

//------------------------------------------------------------------------------
void GDBrowserView::allowDownloads(bool allow)
{
    m_allow_downloads = allow;
}

//------------------------------------------------------------------------------
void GDBrowserView::setDownloadFolder(godot::String path)
{
    if (path.begins_with("user://") || path.begins_with("res://"))
    {
        m_download_folder = GLOBALIZE_PATH(path);
    }
    else
    {
        m_download_folder = path.utf8().get_data();
    }
}

//------------------------------------------------------------------------------
void GDBrowserView::downloadFile(godot::String url)
{
    m_browser->GetHost()->StartDownload(url.utf8().get_data());
}

//------------------------------------------------------------------------------
bool GDBrowserView::canDownload(CefRefPtr<CefBrowser> browser,
                                const CefString& url,
                                const CefString& request_method)
{
    // TODO add a whitelist ["*"]
    return m_allow_downloads;
}

//------------------------------------------------------------------------------
bool GDBrowserView::onBeforeDownload(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback)
{
    fs::path download_path =
        fs::path(m_download_folder) / fs::path(suggested_name.c_str());
    BROWSER_DEBUG("Downloading file for path " << download_path.string());

    // Don't show the download dialog, just go for it
    callback->Continue(download_path.string().c_str(), false);

    return false;
}

//------------------------------------------------------------------------------
void GDBrowserView::onDownloadUpdated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback)
{
    int percentage = download_item->GetPercentComplete();
    std::string file = download_item->GetFullPath();

    BROWSER_DEBUG("Download " << file << " Updated: " << percentage);

    // m_renderHandler->parentUI->DownloadUpdated.Broadcast(url, percentage);
    if ((percentage == 100) && (download_item->IsComplete()))
    {
        BROWSER_DEBUG("Download " << file << " Complete");
        // m_renderHandler->parentUI->DownloadComplete.Broadcast(url);
    }

    // Example download cancel/pause etc, we just have to hijack this
    // callback->Cancel();

    // Emit signal for Godot script
    emit_signal(
        "on_download_updated", godot::String(file.c_str()), percentage, this);
}

//------------------------------------------------------------------------------
bool GDBrowserView::registerGodotMethod(const godot::Callable& callable)
{
    godot::String method_name = callable.get_method();
    BROWSER_DEBUG("Registering gdscript method "
                  << method_name.utf8().get_data());

    if (!callable.is_valid())
    {
        BROWSER_ERROR("Invalid callable gdscript method "
                      << method_name.utf8().get_data());
        return false;
    }

    std::string key = method_name.utf8().get_data();
    m_js_bindings[key] = callable;
    return true;
}

//------------------------------------------------------------------------------
bool GDBrowserView::onProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
    BROWSER_DEBUG("Received message " << message->GetName().ToString());
    if (message->GetName() != "callGodotMethod")
    {
        BROWSER_DEBUG("Expecting IPC command 'callGodotMethod'");
        return false;
    }

    if (message->GetArgumentList()->GetSize() < 1)
    {
        BROWSER_ERROR("Expected method name as first argument for "
                      "'callGodotMethod' IPC command");
        return false;
    }

    // Create the Godot Callable key
    std::string key = message->GetArgumentList()->GetString(0).ToString();

    // Does the Godot Callable exist ?
    auto callable = m_js_bindings[key];
    if (!callable.is_valid())
    {
        BROWSER_ERROR("Callable not found for method " << key);
        return false;
    }

    // Convert the IPC message arguments to a Godot Array
    godot::Array args;
    auto message_args = message->GetArgumentList();
    for (size_t i = 1; i < message_args->GetSize(); ++i)
    {
        switch (message_args->GetType(i))
        {
            case VTYPE_BOOL:
                args.push_back(message_args->GetBool(i));
                break;
            case VTYPE_INT:
                args.push_back(message_args->GetInt(i));
                break;
            case VTYPE_DOUBLE:
                args.push_back(message_args->GetDouble(i));
                break;
            case VTYPE_STRING:
                args.push_back(godot::String(
                    message_args->GetString(i).ToString().c_str()));
                break;
            default:
                // For unsupported types, pass as string
                args.push_back(godot::String(
                    message_args->GetString(i).ToString().c_str()));
                break;
        }
    }

    // Call the function
    callable.callv(args);
    return true;
}

//------------------------------------------------------------------------------
bool GDBrowserView::sendToJS(godot::String eventName,
                             const godot::Variant& data)
{
    BROWSER_DEBUG("Sending event '" << eventName.utf8().get_data() << "'");

    if (!m_browser || !m_browser->GetMainFrame())
    {
        BROWSER_ERROR("Browser not ready");
        return false;
    }

    // Create message
    CefRefPtr<CefProcessMessage> message =
        CefProcessMessage::Create("GodotToJS");
    CefRefPtr<CefListValue> args = message->GetArgumentList();

    // Add event name using the helper function
    args->SetString(0, eventName.utf8().get_data());

    // Convert Godot Variant to CefValue and add it
    // directly to args
    CefRefPtr<CefValue> cef_data = GodotToCefVal(data);
    if (!cef_data)
    {
        BROWSER_ERROR("Failed to convert Godot data to CefValue");
        return false;
    }

    // Add the CefValue directly to arguments
    args->SetValue(1, cef_data);

    // Send to render process
    BROWSER_DEBUG("Sending message to render process");
    m_browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
    return true;
}