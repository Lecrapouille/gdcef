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
#include "helper_config.hpp"
#include "helper_files.hpp"
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/input.hpp>
#include <fstream>

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <base/cef_logging.h>

#ifdef _OPENMP
#    include <omp.h>
#    define OPENMP_PARALLEL_FOR #pragma omp parallel for
#    define PARALLEL_FOR OPENMP_PARALLEL_FOR for
#else
#    define PARALLEL_FOR for
#endif

// SIMD optimization for BGRA->RGBA conversion
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#    include <emmintrin.h>  // SSE2
#    if defined(__SSSE3__) || (defined(_MSC_VER) && defined(__AVX__))
#        include <tmmintrin.h>  // SSSE3 for _mm_shuffle_epi8
#        define GDCEF_USE_SSSE3 1
#    endif
#    define GDCEF_USE_SSE2 1
#endif

//------------------------------------------------------------------------------
// SIMD-optimized BGRA to RGBA conversion (processes 4 pixels at once)
// This is ~3-4x faster than the scalar version
//------------------------------------------------------------------------------
#ifdef GDCEF_USE_SSSE3
static inline void convertBGRAtoRGBA_SIMD(unsigned char* dst,
                                          const unsigned char* src,
                                          int pixelCount)
{
    // Shuffle mask for BGRA -> RGBA: for each 4-byte pixel, swap bytes 0 and 2
    // BGRA = [B G R A] -> RGBA = [R G B A]
    // Indices: 2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15
    const __m128i shuffleMask = _mm_setr_epi8(
        2, 1, 0, 3,    // Pixel 0: BGRA -> RGBA
        6, 5, 4, 7,    // Pixel 1: BGRA -> RGBA
        10, 9, 8, 11,  // Pixel 2: BGRA -> RGBA
        14, 13, 12, 15 // Pixel 3: BGRA -> RGBA
    );

    int simdPixels = pixelCount & ~3;  // Round down to multiple of 4
    int i = 0;

    // Process 4 pixels (16 bytes) at a time
    for (; i < simdPixels; i += 4)
    {
        __m128i bgra = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i * 4));
        __m128i rgba = _mm_shuffle_epi8(bgra, shuffleMask);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i * 4), rgba);
    }

    // Handle remaining pixels (0-3)
    for (; i < pixelCount; ++i)
    {
        int offset = i * 4;
        dst[offset + 0] = src[offset + 2];  // R <- B
        dst[offset + 1] = src[offset + 1];  // G <- G
        dst[offset + 2] = src[offset + 0];  // B <- R
        dst[offset + 3] = src[offset + 3];  // A <- A
    }
}
#endif

//------------------------------------------------------------------------------
// Scalar fallback for BGRA to RGBA conversion
//------------------------------------------------------------------------------
static inline void convertBGRAtoRGBA_Scalar(unsigned char* dst,
                                             const unsigned char* src,
                                             int pixelCount)
{
    for (int i = 0; i < pixelCount; ++i)
    {
        int offset = i * 4;
        dst[offset + 0] = src[offset + 2];  // R <- B
        dst[offset + 1] = src[offset + 1];  // G <- G
        dst[offset + 2] = src[offset + 0];  // B <- R
        dst[offset + 3] = src[offset + 3];  // A <- A
    }
}

//------------------------------------------------------------------------------
// Main conversion function - uses SIMD if available
//------------------------------------------------------------------------------
static inline void convertBGRAtoRGBA(unsigned char* dst,
                                      const unsigned char* src,
                                      int pixelCount)
{
#ifdef GDCEF_USE_SSSE3
    convertBGRAtoRGBA_SIMD(dst, src, pixelCount);
#else
    convertBGRAtoRGBA_Scalar(dst, src, pixelCount);
#endif
}

//------------------------------------------------------------------------------
// Visit the html content of the current page and emit signal with content.
class HtmlContentVisitor : public CefStringVisitor
{
public:

    HtmlContentVisitor(GdBrowserView& node) : m_node(node) {}

    virtual void Visit(const CefString& string) override
    {
        godot::String html(string.ToString().c_str());
        m_node.emit_signal("on_html_content_requested", html, &m_node);
    }

private:

    GdBrowserView& m_node;
    IMPLEMENT_REFCOUNTING(HtmlContentVisitor);
};

//------------------------------------------------------------------------------
// Visit the html content and save it to a file.
class SavePageVisitor : public CefStringVisitor
{
public:

    SavePageVisitor(GdBrowserView& node, const godot::String& path)
        : m_node(node), m_path(path) {}

    virtual void Visit(const CefString& string) override
    {
        std::string html = string.ToString();
        std::string filepath = m_path.utf8().get_data();

        // Try to save to file using C++ standard library
        std::ofstream file(filepath);
        bool success = false;

        if (file.is_open())
        {
            file << html;
            file.close();
            success = true;
            GDCEF_DEBUG("Page saved to: " << filepath);
        }
        else
        {
            GDCEF_ERROR("Failed to save page to: " << filepath);
        }

        m_node.emit_signal("on_page_saved", m_path, success, &m_node);
    }

private:

    GdBrowserView& m_node;
    godot::String m_path;
    IMPLEMENT_REFCOUNTING(SavePageVisitor);
};

//------------------------------------------------------------------------------
// Callback for PDF printing completion.
class PdfPrintCallback : public CefPdfPrintCallback
{
public:

    PdfPrintCallback(GdBrowserView& node, const godot::String& path)
        : m_node(node), m_path(path) {}

    virtual void OnPdfPrintFinished(const CefString& path, bool ok) override
    {
        GDCEF_DEBUG("PDF save " << (ok ? "succeeded" : "failed")
                    << ": " << path.ToString());
        m_node.emit_signal("on_pdf_saved", m_path, ok, &m_node);
    }

private:

    GdBrowserView& m_node;
    godot::String m_path;
    IMPLEMENT_REFCOUNTING(PdfPrintCallback);
};

//------------------------------------------------------------------------------
GdBrowserView::Impl::~Impl()
{
    GDCEF_DEBUG("[gdCEF][GdBrowserView::Impl::~Impl] destroying browser");
}

//------------------------------------------------------------------------------
// in a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method CefRefPtr<CefBrowser> m_browser; this is used to expose various
// methods of this class to Godot.
void GdBrowserView::_bind_methods()
{
    GDCEF_DEBUG("[gdCEF][GdBrowserView::_bind_methods]");

    using namespace godot;

    // Methods
    ClassDB::bind_method(D_METHOD("close"), &GdBrowserView::close);
    ClassDB::bind_method(D_METHOD("id"), &GdBrowserView::id);
    ClassDB::bind_method(D_METHOD("get_error"), &GdBrowserView::getError);
    ClassDB::bind_method(D_METHOD("is_valid"), &GdBrowserView::isValid);
    ClassDB::bind_method(D_METHOD("set_texture", "texture"),
                         &GdBrowserView::setTexture);
    ClassDB::bind_method(D_METHOD("get_texture"), &GdBrowserView::getTexture);
    ClassDB::bind_method(D_METHOD("set_zoom_level"),
                         &GdBrowserView::setZoomLevel);
    ClassDB::bind_method(D_METHOD("get_title"), &GdBrowserView::getTitle);
    ClassDB::bind_method(D_METHOD("get_url"), &GdBrowserView::getURL);
    ClassDB::bind_method(D_METHOD("load_url"), &GdBrowserView::loadURL);
    ClassDB::bind_method(D_METHOD("load_data_uri"),
                         &GdBrowserView::loadDataURI);
    ClassDB::bind_method(D_METHOD("download_file"),
                         &GdBrowserView::downloadFile);
    ClassDB::bind_method(D_METHOD("allow_downloads"),
                         &GdBrowserView::allowDownloads);
    ClassDB::bind_method(D_METHOD("set_download_folder"),
                         &GdBrowserView::setDownloadFolder);
    ClassDB::bind_method(D_METHOD("is_loaded"), &GdBrowserView::loaded);
    ClassDB::bind_method(D_METHOD("reload"), &GdBrowserView::reload);
    ClassDB::bind_method(D_METHOD("stop_loading"), &GdBrowserView::stopLoading);
    ClassDB::bind_method(D_METHOD("copy"), &GdBrowserView::copy);
    ClassDB::bind_method(D_METHOD("paste"), &GdBrowserView::paste);
    ClassDB::bind_method(D_METHOD("undo"), &GdBrowserView::undo);
    ClassDB::bind_method(D_METHOD("redo"), &GdBrowserView::redo);
    ClassDB::bind_method(D_METHOD("request_html_content"),
                         &GdBrowserView::requestHtmlContent);
    ClassDB::bind_method(D_METHOD("save_page", "path"),
                         &GdBrowserView::savePage);
    ClassDB::bind_method(D_METHOD("save_page_as_pdf", "path"),
                         &GdBrowserView::savePageAsPdf);
    ClassDB::bind_method(D_METHOD("has_previous_page"),
                         &GdBrowserView::canNavigateBackward);
    ClassDB::bind_method(D_METHOD("has_next_page"),
                         &GdBrowserView::canNavigateForward);
    ClassDB::bind_method(D_METHOD("previous_page"),
                         &GdBrowserView::navigateBackward);
    ClassDB::bind_method(D_METHOD("next_page"),
                         &GdBrowserView::navigateForward);
    ClassDB::bind_method(D_METHOD("resize"), &GdBrowserView::resize);
    ClassDB::bind_method(D_METHOD("set_viewport"), &GdBrowserView::viewport);
    ClassDB::bind_method(D_METHOD("set_key_pressed"), &GdBrowserView::keyPress);
    ClassDB::bind_method(D_METHOD("set_mouse_moved"),
                         &GdBrowserView::mouseMove);
    ClassDB::bind_method(D_METHOD("set_mouse_left_click"),
                         &GdBrowserView::leftClick);
    ClassDB::bind_method(D_METHOD("set_mouse_right_click"),
                         &GdBrowserView::rightClick);
    ClassDB::bind_method(D_METHOD("set_mouse_middle_click"),
                         &GdBrowserView::middleClick);
    ClassDB::bind_method(D_METHOD("set_mouse_left_down"),
                         &GdBrowserView::leftMouseDown);
    ClassDB::bind_method(D_METHOD("set_mouse_left_up"),
                         &GdBrowserView::leftMouseUp);
    ClassDB::bind_method(D_METHOD("set_mouse_right_down"),
                         &GdBrowserView::rightMouseDown);
    ClassDB::bind_method(D_METHOD("set_mouse_right_up"),
                         &GdBrowserView::rightMouseUp);
    ClassDB::bind_method(D_METHOD("set_mouse_middle_down"),
                         &GdBrowserView::middleMouseDown);
    ClassDB::bind_method(D_METHOD("set_mouse_middle_up"),
                         &GdBrowserView::middleMouseUp);
    ClassDB::bind_method(D_METHOD("set_mouse_wheel_vertical", "delta", "shift",
                                   "ctrl", "alt"),
                         &GdBrowserView::mouseWheelVertical,
                         DEFVAL(false), DEFVAL(false), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("set_mouse_wheel_horizontal", "delta", "shift",
                                   "ctrl", "alt"),
                         &GdBrowserView::mouseWheelHorizontal,
                         DEFVAL(false), DEFVAL(false), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("set_touch_down", "id", "x", "y"),
                         &GdBrowserView::touchDown);
    ClassDB::bind_method(D_METHOD("set_touch_move", "id", "x", "y"),
                         &GdBrowserView::touchMove);
    ClassDB::bind_method(D_METHOD("set_touch_up", "id", "x", "y"),
                         &GdBrowserView::touchUp);
    ClassDB::bind_method(D_METHOD("set_touch_cancel", "id"),
                         &GdBrowserView::touchCancel);
    ClassDB::bind_method(D_METHOD("set_muted"), &GdBrowserView::mute);
    ClassDB::bind_method(D_METHOD("is_muted"), &GdBrowserView::muted);
    ClassDB::bind_method(D_METHOD("set_audio_stream", "audio"),
                         &GdBrowserView::setAudioStreamer);
    ClassDB::bind_method(D_METHOD("get_audio_stream"),
                         &GdBrowserView::getAudioStreamer);
    ClassDB::bind_method(D_METHOD("get_pixel_color", "x", "y"),
                         &GdBrowserView::getPixelColor);
    ClassDB::bind_method(D_METHOD("register_method", "object", "method"),
                         &GdBrowserView::registerGodotMethod);
    ClassDB::bind_method(D_METHOD("js_emit", "event_name", "data"),
                         &GdBrowserView::jsEmit);
    ClassDB::bind_method(D_METHOD("execute_javascript"),
                         &GdBrowserView::executeJavaScript);
    ClassDB::bind_method(D_METHOD("add_ad_block_pattern", "pattern"),
                         &GdBrowserView::addAdBlockPattern);
    ClassDB::bind_method(D_METHOD("enable_ad_block", "enable"),
                         &GdBrowserView::enableAdBlock);
    ClassDB::bind_method(D_METHOD("is_ad_block_enabled"),
                         &GdBrowserView::isAdBlockEnabled);
    ClassDB::bind_method(D_METHOD("log_info", "message"), &GdBrowserView::log_info);
    ClassDB::bind_method(D_METHOD("log_warning", "message"), &GdBrowserView::log_warning);
    ClassDB::bind_method(D_METHOD("log_error", "message"), &GdBrowserView::log_error);
    ClassDB::bind_method(D_METHOD("log_fatal", "message"), &GdBrowserView::log_fatal);

    // Drag and drop methods
    ClassDB::bind_method(D_METHOD("enable_drag_and_drop", "enable"),
                         &GdBrowserView::enableDragAndDrop);
    ClassDB::bind_method(D_METHOD("is_drag_and_drop_enabled"),
                         &GdBrowserView::isDragAndDropEnabled);
    ClassDB::bind_method(D_METHOD("drag_enter", "x", "y", "text", "html", "url"),
                         &GdBrowserView::dragEnter);
    ClassDB::bind_method(D_METHOD("drag_over", "x", "y"),
                         &GdBrowserView::dragOver);
    ClassDB::bind_method(D_METHOD("drag_leave"), &GdBrowserView::dragLeave);
    ClassDB::bind_method(D_METHOD("drop", "x", "y"), &GdBrowserView::drop);
    ClassDB::bind_method(D_METHOD("is_dragging"), &GdBrowserView::isDragging);
    ClassDB::bind_method(D_METHOD("end_dragging", "x", "y"),
                         &GdBrowserView::endDragging);

    // Signals
    ADD_SIGNAL(MethodInfo("on_download_updated",
                          PropertyInfo(Variant::STRING, "file"),
                          PropertyInfo(Variant::INT, "percentage"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_page_loaded",
                          PropertyInfo(Variant::INT, "http_code"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_page_start_loading",
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_page_failed_loading",
                          PropertyInfo(Variant::INT, "err_code"),
                          PropertyInfo(Variant::STRING, "err_msg"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_browser_paint",
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_html_content_requested",
                          PropertyInfo(Variant::STRING, "html"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_page_saved",
                          PropertyInfo(Variant::STRING, "path"),
                          PropertyInfo(Variant::BOOL, "success"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_pdf_saved",
                          PropertyInfo(Variant::STRING, "path"),
                          PropertyInfo(Variant::BOOL, "success"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_drag_enter",
                          PropertyInfo(Variant::DICTIONARY, "drag_info"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_draggable_regions_changed",
                          PropertyInfo(Variant::ARRAY, "regions"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_start_dragging",
                          PropertyInfo(Variant::DICTIONARY, "drag_info"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_update_drag_cursor",
                          PropertyInfo(Variant::INT, "operation"),
                          PropertyInfo(Variant::OBJECT, "browser")));
    ADD_SIGNAL(MethodInfo("on_cursor_changed",
                          PropertyInfo(Variant::INT, "cursor_shape"),
                          PropertyInfo(Variant::OBJECT, "browser")));

    // Properties.
    //
    // Note: the browser texture is deliberately not exposed as a property. It
    // is an output owned by the browser (created and resized by onPaint()) and
    // shall be read with get_texture(). Exposing it also made Godot warn about
    // an instantiated ImageTexture used as class default value, because the
    // engine snapshots the default value of every editor/storage property from
    // a throw-away instance of the class.
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT,
                              "audio_stream",
                              PROPERTY_HINT_RESOURCE_TYPE,
                              "AudioStreamGeneratorPlayback"),
                 "set_audio_stream",
                 "get_audio_stream");
}

//------------------------------------------------------------------------------
void GdBrowserView::_init()
{
    BROWSER_DEBUG("");
}

//------------------------------------------------------------------------------
godot::String GdBrowserView::getError()
{
    std::string err = m_error.str();
    m_error.clear();
    return {err.c_str()};
}

//------------------------------------------------------------------------------
int GdBrowserView::init(godot::String const& url,
                        CefBrowserSettings const& settings,
                        CefWindowInfo const& window_info)
{
    // Allocate the CEF client and the Godot rendering resources here instead of
    // in the constructor: Godot also builds throw-away instances of registered
    // classes (to collect the default value of the properties or to generate
    // the class documentation) and none of this is needed for them.
    if (m_impl == nullptr)
    {
        m_impl = new GdBrowserView::Impl(*this);
        if (m_impl == nullptr)
        {
            GDCEF_ERROR("Failed allocating GdBrowserView::Impl");
            return -1;
        }
    }
    if (!m_texture.is_valid())
    {
        m_texture.instantiate();
    }

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
GdBrowserView::GdBrowserView() : m_viewport({0.0f, 0.0f, 1.0f, 1.0f})
{
    BROWSER_DEBUG("Creating new GdBrowserView");
}

//------------------------------------------------------------------------------
GdBrowserView::~GdBrowserView()
{
    close();
}

//------------------------------------------------------------------------------
void GdBrowserView::getViewRect(CefRefPtr<CefBrowser> /*browser*/,
                                CefRect& rect)
{
    rect = CefRect(int(m_viewport[0] * m_width),
                   int(m_viewport[1] * m_height),
                   int(m_viewport[2] * m_width),
                   int(m_viewport[3] * m_height));
}

//------------------------------------------------------------------------------
void GdBrowserView::onPaint(CefRefPtr<CefBrowser> /*browser*/,
                            CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height)
{
    // CEF renders native popup widgets (i.e. an expanded <select> list) in
    // their own and smaller buffer. We do not composite them over the page, so
    // ignore them: else the page texture would be replaced by the popup bitmap
    // until the next PET_VIEW paint. Compositing them would mean implementing
    // CefRenderHandler::OnPopupShow() and OnPopupSize() to know where to blit
    // this buffer over the page.
    if (type != PET_VIEW)
        return;

    // Sanity check
    if ((width <= 0) || (height <= 0) || (buffer == nullptr))
        return;

    // BGRA8: blue, green, red components each coded as byte
    int const COLOR_CHANELS = 4;
    int const SIZEOF_COLOR = COLOR_CHANELS * sizeof(char);
    int const TEXTURE_SIZE = SIZEOF_COLOR * width * height;

    // Compare the dimension and not the size of the buffer: a resize keeping
    // the pixel count (i.e. 400 x 300 becoming 300 x 400) would else be missed
    // and ImageTexture::update() refuses an image of different dimension.
    bool bResized = (width != m_painted_width) || (height != m_painted_height);
    m_painted_width = width;
    m_painted_height = height;

    // Copy CEF image buffer to Godot PoolByteArray
    m_data.resize(TEXTURE_SIZE);

    // Copy per line func for OpenMP/PPL - uses SIMD-optimized conversion
    unsigned char* imageData = m_data.ptrw();
    const unsigned char* cbuffer = (const unsigned char*)buffer;
    auto doCopyLine = [imageData, cbuffer, width](int line, int x, int copyWidth) {
        int pixelOffset = line * width + x;
        convertBGRAtoRGBA(
            imageData + pixelOffset * 4,
            cbuffer + pixelOffset * 4,
            copyWidth);
    };

    if (bResized)
    {
        PARALLEL_FOR(int y = 0; y < height; ++y)
        {
            doCopyLine(y, 0, width);
        }
    }
    else
    {
        for (const CefRect& rect : dirtyRects)
        {
            PARALLEL_FOR(int y = rect.y; y < rect.y + rect.height; ++y)
            {
                doCopyLine(y, rect.x, rect.width);
            }
        }
    }

    // Wrap the pixels inside a Godot image to upload them to the Godot texture.
    // This image is deliberately temporary: holding it as a member would hold a
    // reference on m_data, and Godot's copy-on-write would then duplicate the
    // whole frame at the first write of the next paint, making the partial copy
    // of the dirty rectangles pointless.
    godot::Ref<godot::Image> image = godot::Image::create_from_data(
        width, height, false, godot::Image::FORMAT_RGBA8, m_data);

    if (bResized)
    {
        // ImageTexture::update() only accepts an image of the dimension of the
        // texture, so the texture has to be recreated.
        m_texture->set_image(image);
    }
    else
    {
        m_texture->update(image);
    }

    emit_signal("on_browser_paint", this);
}

//------------------------------------------------------------------------------
void GdBrowserView::onLoadStart(CefRefPtr<CefBrowser> /*browser*/,
                                CefRefPtr<CefFrame> frame)
{
    // Emit signal only when top-level frame is loading.
    if (frame->IsMain())
    {
        BROWSER_DEBUG("has started loading " << frame->GetURL());

        // Emit signal for Godot script
        emit_signal("on_page_start_loading", this);
    }
}

//------------------------------------------------------------------------------
void GdBrowserView::onLoadEnd(CefRefPtr<CefBrowser> /*browser*/,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
    // Emit signal only when top-level frame was loaded.
    if (frame->IsMain())
    {
        BROWSER_DEBUG("has ended loading " << frame->GetURL());

        // Emit signal for Godot script
        emit_signal("on_page_loaded", httpStatusCode, this);
    }
}

//------------------------------------------------------------------------------
void GdBrowserView::onLoadError(CefRefPtr<CefBrowser> /*browser*/,
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
void GdBrowserView::setZoomLevel(double delta)
{
    BROWSER_DEBUG(delta);

    if (!m_browser)
        return;

    m_browser->GetHost()->SetZoomLevel(delta);
}

//------------------------------------------------------------------------------
void GdBrowserView::loadURL(godot::String url)
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
void GdBrowserView::loadDataURI(godot::String html, godot::String mime_type)
{
    auto const& d = html.utf8();
    std::string uri("data:");
    uri += mime_type.utf8().get_data();
    uri += ";base64,";
    uri += CefURIEncode(CefBase64Encode(d.ptr(), d.length()), false).ToString();
    m_browser->GetMainFrame()->LoadURL(uri);
}

//------------------------------------------------------------------------------
bool GdBrowserView::reload() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    m_browser->Reload();
    return true;
}

//------------------------------------------------------------------------------
bool GdBrowserView::loaded() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    return m_browser->HasDocument();
}

//------------------------------------------------------------------------------
godot::String GdBrowserView::getURL() const
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
godot::String GdBrowserView::getTitle() const
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
void GdBrowserView::stopLoading()
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return;

    m_browser->StopLoad();
}

//------------------------------------------------------------------------------
// FIXME https://github.com/chromiumembedded/cef/issues/3117
void GdBrowserView::copy() const
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
void GdBrowserView::paste() const
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
void GdBrowserView::cut() const
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
void GdBrowserView::delete_() const
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
void GdBrowserView::undo() const
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
void GdBrowserView::redo() const
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
void GdBrowserView::requestHtmlContent()
{
    if (m_browser && m_browser->GetMainFrame())
    {
        CefRefPtr<HtmlContentVisitor> visitor = new HtmlContentVisitor(*this);
        m_browser->GetMainFrame()->GetSource(visitor);
        return;
    }

    BROWSER_ERROR("Cannot retrieve HTML content: no browser or frame");
}

//------------------------------------------------------------------------------
void GdBrowserView::savePage(godot::String path)
{
    if (m_browser && m_browser->GetMainFrame())
    {
        // Resolve Godot paths (res://, user://) to absolute paths
        godot::String resolved_path = path;
        if (path.begins_with("res://") || path.begins_with("user://"))
        {
            resolved_path = godot::ProjectSettings::get_singleton()
                                ->globalize_path(path);
        }

        CefRefPtr<SavePageVisitor> visitor =
            new SavePageVisitor(*this, resolved_path);
        m_browser->GetMainFrame()->GetSource(visitor);
        return;
    }

    BROWSER_ERROR("Cannot save page: no browser or frame");
    emit_signal("on_page_saved", path, false, this);
}

//------------------------------------------------------------------------------
void GdBrowserView::savePageAsPdf(godot::String path)
{
    if (!m_browser)
    {
        BROWSER_ERROR("Cannot save PDF: no browser");
        emit_signal("on_pdf_saved", path, false, this);
        return;
    }

    // Resolve Godot paths (res://, user://) to absolute paths
    godot::String resolved_path = path;
    if (path.begins_with("res://") || path.begins_with("user://"))
    {
        resolved_path = godot::ProjectSettings::get_singleton()
                            ->globalize_path(path);
    }

    // Configure PDF settings (A4 paper size)
    CefPdfPrintSettings settings;
    settings.landscape = 0;                    // Portrait mode
    settings.print_background = 1;             // Include background graphics
    settings.scale = 1.0;                      // 100% scale
    settings.paper_width = 8.27;               // A4 width in inches (210mm)
    settings.paper_height = 11.69;             // A4 height in inches (297mm)
    settings.margin_type = PDF_PRINT_MARGIN_DEFAULT;
    settings.display_header_footer = 0;        // No header/footer

    CefRefPtr<PdfPrintCallback> callback =
        new PdfPrintCallback(*this, resolved_path);

    m_browser->GetHost()->PrintToPDF(
        resolved_path.utf8().get_data(), settings, callback);

    BROWSER_DEBUG("Saving page as PDF to: " << resolved_path.utf8().get_data());
}

//------------------------------------------------------------------------------
void GdBrowserView::executeJavaScript(godot::String javascript)
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
bool GdBrowserView::canNavigateBackward() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    return m_browser->CanGoBack();
}

//------------------------------------------------------------------------------
void GdBrowserView::navigateBackward()
{
    BROWSER_DEBUG("");

    if ((m_browser != nullptr) && (m_browser->CanGoBack()))
    {
        m_browser->GoBack();
    }
}

//------------------------------------------------------------------------------
bool GdBrowserView::canNavigateForward() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    return m_browser->CanGoForward();
}

//------------------------------------------------------------------------------
void GdBrowserView::navigateForward()
{
    BROWSER_DEBUG("");

    if ((m_browser != nullptr) && (m_browser->CanGoForward()))
    {
        m_browser->GoForward();
    }
}

//------------------------------------------------------------------------------
void GdBrowserView::resize_(int width, int height)
{
    if (width <= 0)
    {
        width = 2;
    }
    if (height <= 0)
    {
        height = 2;
    }

    // Avoid calling WasResized() if size hasn't changed (prevents resize loops)
    if (m_width == float(width) && m_height == float(height))
    {
        return;
    }

    BROWSER_DEBUG(width << " x " << height);

    m_width = float(width);
    m_height = float(height);

    if (!m_browser || !m_browser->GetHost())
        return;

    m_browser->GetHost()->WasResized();
}

//------------------------------------------------------------------------------
bool GdBrowserView::viewport(float x, float y, float w, float h)
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
bool GdBrowserView::isValid() const
{
    BROWSER_DEBUG("");

    if (!m_browser)
        return false;

    return m_browser->IsValid();
}

//------------------------------------------------------------------------------
void GdBrowserView::close()
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
bool GdBrowserView::mute(bool mute)
{
    CEF_REQUIRE_UI_THREAD();
    if (m_browser == nullptr)
        return true;

    m_browser->GetHost()->SetAudioMuted(mute);
    return m_browser->GetHost()->IsAudioMuted();
}

//------------------------------------------------------------------------------
bool GdBrowserView::muted()
{
    CEF_REQUIRE_UI_THREAD();
    if (m_browser == nullptr)
        return true;

    return m_browser->GetHost()->IsAudioMuted();
}

//------------------------------------------------------------------------------
void GdBrowserView::onAudioStreamStarted(CefRefPtr<CefBrowser> browser,
                                         const CefAudioParameters& params,
                                         int channels)
{
    if (m_impl == nullptr)
        return;

    m_impl->m_audio.channels = int(params.channel_layout);
}

//------------------------------------------------------------------------------
void GdBrowserView::onAudioStreamPacket(CefRefPtr<CefBrowser> browser,
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
godot::Color GdBrowserView::getPixelColor(int x, int y) const
{
    // Bound the coordinates with the dimension of the painted buffer and not
    // with m_width and m_height: those are the desired dimension, applied by
    // CEF only at its next paint, so they can be larger than m_data which would
    // make us read out of its bounds.
    if ((x < 0) || (y < 0) || (x >= m_painted_width) || (y >= m_painted_height))
        return godot::Color(1, 1, 1, 1); // Return full white as fallback

    int index = (y * m_painted_width + x) * 4;
    unsigned char r = m_data[index + 0];
    unsigned char g = m_data[index + 1];
    unsigned char b = m_data[index + 2];
    unsigned char a = m_data[index + 3];

    return godot::Color(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
}

//------------------------------------------------------------------------------
bool GdBrowserView::onBeforePopup(CefRefPtr<CefBrowser> browser,
                                  const CefString& target_url)
{
    // Prevent opening page on new windows.
    // See: https://github.com/Lecrapouille/gdcef/issues/19
    browser->GetMainFrame()->LoadURL(target_url);
    return true;
}

//------------------------------------------------------------------------------
void GdBrowserView::allowDownloads(bool allow)
{
    m_allow_downloads = allow;
}

//------------------------------------------------------------------------------
void GdBrowserView::setDownloadFolder(godot::String path)
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
void GdBrowserView::downloadFile(godot::String url)
{
    m_browser->GetHost()->StartDownload(url.utf8().get_data());
}

//------------------------------------------------------------------------------
bool GdBrowserView::canDownload(CefRefPtr<CefBrowser> browser,
                                const CefString& url,
                                const CefString& request_method)
{
    // TODO add a whitelist ["*"]
    return m_allow_downloads;
}

//------------------------------------------------------------------------------
bool GdBrowserView::onBeforeDownload(
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
void GdBrowserView::onDownloadUpdated(
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
bool GdBrowserView::registerGodotMethod(godot::Object* object,
                                        godot::String method_name)
{
    BROWSER_DEBUG("Registering gdscript method "
                  << method_name.utf8().get_data());

    godot::Callable callable(object, method_name);
    if (!callable.is_valid())
    {
        BROWSER_ERROR("Invalid callable gdscript method "
                      << method_name.utf8().get_data());
        return false;
    }

    // Register the method
    if (method_name.begins_with("_"))
    {
        m_js_bindings[method_name.substr(1).utf8().get_data()] = callable;
    }
    else
    {
        m_js_bindings[method_name.utf8().get_data()] = callable;
    }

    return true;
}

//------------------------------------------------------------------------------
// Recursive method to convert JSON data to native Godot types
godot::Variant GdBrowserView::JsonToGodot(const godot::Dictionary& json)
{
    // Special case for binary data encoded in base64
    if (json.has("type") && json["type"] == "binary" && json.has("format") &&
        json["format"] == "base64" && json.has("data") && json.has("size"))
    {
        godot::String base64_data = json["data"];
        int expected_size = json["size"];

        // Decode base64 to binary data
        std::string base64_str = base64_data.utf8().get_data();

        // Get binary data
        CefRefPtr<CefBinaryValue> binary = CefBase64Decode(base64_str);
        if (binary.get() && binary->GetSize() > 0)
        {
            // Convert to Godot PackedByteArray
            godot::PackedByteArray byte_array;
            byte_array.resize(binary->GetSize());

            // Copy decoded data to Godot byte array
            binary->GetData(byte_array.ptrw(), binary->GetSize(), 0);

            if (byte_array.size() != expected_size)
            {
                BROWSER_DEBUG("Binary size mismatch: expected "
                              << expected_size << " but got "
                              << byte_array.size());
            }

            return byte_array;
        }
        else
        {
            BROWSER_ERROR("Failed to decode base64 data");
            return godot::Variant();
        }
    }

    // Recursive processing of nested JSON structures
    godot::Dictionary result;

    // Iterate over all elements of the dictionary
    godot::Array keys = json.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        godot::Variant key = keys[i];
        godot::Variant value = json[key];

        // Recursive processing of nested dictionaries
        if (value.get_type() == godot::Variant::Type::DICTIONARY)
        {
            godot::Dictionary dict = value;
            result[key] = JsonToGodot(dict);
        }
        // Recursive processing of arrays
        else if (value.get_type() == godot::Variant::Type::ARRAY)
        {
            godot::Array array = value;
            result[key] = JsonToGodot(array);
        }
        // Simple types are copied directly
        else
        {
            result[key] = value;
        }
    }

    return result;
}

//------------------------------------------------------------------------------
// Overload to handle a Godot Array directly
godot::Variant GdBrowserView::JsonToGodot(const godot::Array& json_array)
{
    godot::Array result;
    result.resize(json_array.size());

    for (int i = 0; i < json_array.size(); i++)
    {
        godot::Variant element = json_array[i];

        if (element.get_type() == godot::Variant::Type::DICTIONARY)
        {
            godot::Dictionary dict = element;
            result[i] = JsonToGodot(dict);
        }
        else if (element.get_type() == godot::Variant::Type::ARRAY)
        {
            godot::Array arr = element;
            result[i] = JsonToGodot(arr);
        }
        else
        {
            result[i] = element;
        }
    }

    return result;
}

//------------------------------------------------------------------------------
// Generic overload to handle all Godot Variant types
godot::Variant GdBrowserView::JsonToGodot(const godot::Variant& json_value)
{
    switch (json_value.get_type())
    {
        case godot::Variant::Type::DICTIONARY: {
            godot::Dictionary dict = json_value;
            return JsonToGodot(dict);
        }

        case godot::Variant::Type::ARRAY: {
            godot::Array arr = json_value;
            return JsonToGodot(arr);
        }

        case godot::Variant::Type::NIL:
        case godot::Variant::Type::BOOL:
        case godot::Variant::Type::INT:
        case godot::Variant::Type::FLOAT:
        case godot::Variant::Type::STRING:
        default:
            // Primitive types: return as is
            return json_value;
    }
}

//------------------------------------------------------------------------------
bool GdBrowserView::onProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
    BROWSER_DEBUG("Received message " << message->GetName().ToString());

    // Check that this is a callGodotMethod message
    if (message->GetName() != "callGodotMethod")
    {
        BROWSER_DEBUG("Expecting IPC command 'callGodotMethod'");
        return false;
    }

    // Check that we have at least the method name and arguments
    CefRefPtr<CefListValue> args_list = message->GetArgumentList();
    if (args_list->GetSize() < 2)
    {
        BROWSER_ERROR("Expected method name and JSON arguments for "
                      "'callGodotMethod' IPC command");
        return false;
    }

    // Get the method name
    std::string method_name = args_list->GetString(0).ToString();

    // Check that the method exists in the bindings
    auto callable = m_js_bindings[method_name];
    if (!callable.is_valid())
    {
        BROWSER_ERROR("Callable not found for method " << method_name);
        return false;
    }

    try
    {
        // Get the JSON string of the arguments
        std::string json_args = args_list->GetString(1).ToString();
        godot::String json_godot(json_args.c_str());

        // Parse the JSON to a Godot Variant
        godot::Variant parsed = godot::JSON::parse_string(json_godot);

        // Check that the parsing succeeded
        if (parsed.get_type() == godot::Variant::Type::NIL &&
            !json_godot.is_empty() && json_godot != "null")
        {
            BROWSER_ERROR("Failed to parse JSON arguments: "
                          << json_godot.utf8().get_data());
            return false;
        }

        // GodotMethodHandler::Execute always sends an array
        godot::Array args;

        if (parsed.get_type() == godot::Variant::Type::ARRAY)
        {
            // Convert the types in the received array
            godot::Array array = parsed;
            args = JsonToGodot(array);
        }
        else if (parsed.get_type() != godot::Variant::Type::NIL)
        {
            // Unexpected case, but we handle it: create an array with
            // the element
            args.push_back(JsonToGodot(parsed));
        }

        // Call the function
        callable.callv(args);
        return true;
    }
    catch (const std::exception& e)
    {
        BROWSER_ERROR("Exception in onProcessMessageReceived: " << e.what());
        return false;
    }
}

//------------------------------------------------------------------------------
bool GdBrowserView::jsEmit(godot::String event_name, const godot::Variant& data)
{
    BROWSER_DEBUG("Sending message to render process '"
                  << event_name.utf8().get_data() << "'");

    if (!m_browser || !m_browser->GetMainFrame())
    {
        BROWSER_ERROR("Browser not ready");
        return false;
    }

    try
    {
        // Create IPC message to the render process
        CefRefPtr<CefProcessMessage> message =
            CefProcessMessage::Create("godotEvents.emit");
        CefRefPtr<CefListValue> args = message->GetArgumentList();

        // Add event name
        args->SetString(0, event_name.utf8().get_data());

        // Handle binary data specifically.
        // If the data is a PackedByteArray, serialize it to base64 and
        // convert it to a JSON object since Godot will convert it to a
        // simple array and we want to differentiate from regular array.
        if (data.get_type() == godot::Variant::Type::PACKED_BYTE_ARRAY)
        {
            godot::PackedByteArray binary_data = data;

            // Encode to base64
            std::string base64 =
                CefBase64Encode(binary_data.ptr(), binary_data.size());

            // Create a JSON object with the appropriate properties
            std::string json = "{";
            json += "\"type\":\"binary\",";
            json += "\"format\":\"base64\",";
            json += "\"data\":\"" + base64 + "\",";
            json += "\"size\":" + std::to_string(binary_data.size());
            json += "}";

            args->SetString(1, json);
        }
        else
        {
            // Convert other data types to JSON string
            godot::String json_data = godot::JSON::stringify(data);
            args->SetString(1, json_data.utf8().get_data());
        }

        // Send the message to the render process.
        // The GodotMethodHandler::Execute method in
        // gdcef/subprocess/src/render_process.cpp will be
        // called.
        m_browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
        return true;
    }
    catch (const std::exception& e)
    {
        BROWSER_ERROR("Error sending message to render process: " << e.what());
        return false;
    }
}

//------------------------------------------------------------------------------
bool GdBrowserView::addAdBlockPattern(godot::String pattern)
{
    BROWSER_DEBUG("Adding ad block pattern " << pattern.utf8().get_data());

    if ((m_impl == nullptr) || (m_impl->m_ad_blocker == nullptr))
    {
        BROWSER_ERROR("Ad blocker not initialized");
        return false;
    }

    if (m_impl->m_ad_blocker->addRule(pattern.utf8().get_data()))
    {
        return true;
    }
    else
    {
        BROWSER_ERROR(
            "Invalid ad blocking pattern: " << pattern.utf8().get_data());
        return false;
    }
}

//------------------------------------------------------------------------------
void GdBrowserView::enableAdBlock(bool enable)
{
    BROWSER_DEBUG("Enabling ad blocker " << (enable ? "true" : "false"));

    if ((m_impl == nullptr) || (m_impl->m_ad_blocker == nullptr))
    {
        BROWSER_ERROR("Ad blocker not initialized");
        return;
    }

    m_impl->m_ad_blocker->enable(enable);
}

//------------------------------------------------------------------------------
bool GdBrowserView::isAdBlockEnabled() const
{
    BROWSER_DEBUG("")
    if ((m_impl == nullptr) || (m_impl->m_ad_blocker == nullptr))
    {
        BROWSER_ERROR("Ad blocker not initialized");
        return false;
    }
    return m_impl->m_ad_blocker->is_enabled();
}

//------------------------------------------------------------------------------
void GdBrowserView::log_info(godot::String message)
{
    std::stringstream ss;
    godot::String name = get_name();
    ss << "[browser id: " << m_id << ", name: " << name.utf8().get_data() << "] " << message.utf8().get_data();
    LOG(INFO) << ss.str();
}

//------------------------------------------------------------------------------
void GdBrowserView::log_warning(godot::String message)
{
    std::stringstream ss;
    godot::String name = get_name();
    ss << "[browser id: " << m_id << ", name: " << name.utf8().get_data() << "] " << message.utf8().get_data();
    LOG(WARNING) << ss.str();
}

//------------------------------------------------------------------------------
void GdBrowserView::log_error(godot::String message)
{
    std::stringstream ss;
    godot::String name = get_name();
    ss << "[browser id: " << m_id << ", name: " << name.utf8().get_data() << "] " << message.utf8().get_data();
    LOG(ERROR) << ss.str();
}

//------------------------------------------------------------------------------
void GdBrowserView::log_fatal(godot::String message)
{
    std::stringstream ss;
    godot::String name = get_name();
    ss << "[browser id: " << m_id << ", name: " << name.utf8().get_data() << "] " << message.utf8().get_data();
    LOG(FATAL) << ss.str();
}

//------------------------------------------------------------------------------
void GdBrowserView::enableDragAndDrop(bool enable)
{
    BROWSER_DEBUG("Enabling drag and drop: " << (enable ? "true" : "false"));
    m_drag_and_drop_enabled = enable;
}

//------------------------------------------------------------------------------
bool GdBrowserView::isDragAndDropEnabled() const
{
    return m_drag_and_drop_enabled;
}

//------------------------------------------------------------------------------
bool GdBrowserView::onDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                CefDragHandler::DragOperationsMask mask)
{
    BROWSER_DEBUG("onDragEnter called");

    // If drag and drop is disabled, cancel all drag events
    if (!m_drag_and_drop_enabled)
    {
        BROWSER_DEBUG("Drag and drop disabled, cancelling drag event");
        return true; // Cancel the drag event
    }

    // Emit signal for Godot script with drag information
    godot::Dictionary drag_info;

    // Check what type of data is being dragged
    if (dragData->IsFile())
    {
        drag_info["type"] = "file";
        std::vector<CefString> file_names;
        dragData->GetFileNames(file_names);
        godot::Array files;
        for (const auto& file : file_names)
        {
            files.push_back(godot::String(file.ToString().c_str()));
        }
        drag_info["files"] = files;
    }
    else if (dragData->IsLink())
    {
        drag_info["type"] = "link";
        drag_info["url"] = godot::String(dragData->GetLinkURL().ToString().c_str());
        drag_info["title"] = godot::String(dragData->GetLinkTitle().ToString().c_str());
    }
    else if (dragData->IsFragment())
    {
        drag_info["type"] = "fragment";
        drag_info["text"] = godot::String(dragData->GetFragmentText().ToString().c_str());
        drag_info["html"] = godot::String(dragData->GetFragmentHtml().ToString().c_str());
    }
    else
    {
        drag_info["type"] = "unknown";
    }

    // Add mask information
    drag_info["mask"] = static_cast<int>(mask);

    emit_signal("on_drag_enter", drag_info, this);

    return false; // Allow the drag event (default behavior)
}

//------------------------------------------------------------------------------
void GdBrowserView::onDraggableRegionsChanged(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const std::vector<CefDraggableRegion>& regions)
{
    BROWSER_DEBUG("onDraggableRegionsChanged called with " << regions.size()
                                                           << " regions");

    // Convert regions to Godot format
    godot::Array godot_regions;
    for (const auto& region : regions)
    {
        godot::Dictionary region_dict;
        region_dict["x"] = region.bounds.x;
        region_dict["y"] = region.bounds.y;
        region_dict["width"] = region.bounds.width;
        region_dict["height"] = region.bounds.height;
        region_dict["draggable"] = region.draggable;
        godot_regions.push_back(region_dict);
    }

    emit_signal("on_draggable_regions_changed", godot_regions, this);
}

//------------------------------------------------------------------------------
void GdBrowserView::dragEnter(int x, int y, godot::String text,
                               godot::String html, godot::String url)
{
    BROWSER_DEBUG("dragEnter at " << x << ", " << y);

    if (!m_browser || !m_browser->GetHost())
        return;

    // Create drag data
    m_drag_data = CefDragData::Create();

    if (!text.is_empty())
    {
        m_drag_data->SetFragmentText(text.utf8().get_data());
    }
    if (!html.is_empty())
    {
        m_drag_data->SetFragmentHtml(html.utf8().get_data());
    }
    if (!url.is_empty())
    {
        m_drag_data->SetLinkURL(url.utf8().get_data());
    }

    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = m_mouse_event_modifiers;

    // Notify CEF of the drag enter
    m_browser->GetHost()->DragTargetDragEnter(
        m_drag_data, mouse_event,
        static_cast<CefBrowserHost::DragOperationsMask>(
            DRAG_OPERATION_COPY | DRAG_OPERATION_MOVE | DRAG_OPERATION_LINK));
}

//------------------------------------------------------------------------------
void GdBrowserView::dragOver(int x, int y)
{
    if (!m_browser || !m_browser->GetHost())
        return;

    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->DragTargetDragOver(
        mouse_event,
        static_cast<CefBrowserHost::DragOperationsMask>(
            DRAG_OPERATION_COPY | DRAG_OPERATION_MOVE | DRAG_OPERATION_LINK));
}

//------------------------------------------------------------------------------
void GdBrowserView::dragLeave()
{
    BROWSER_DEBUG("dragLeave");

    if (!m_browser || !m_browser->GetHost())
        return;

    m_browser->GetHost()->DragTargetDragLeave();
    m_drag_data = nullptr;
}

//------------------------------------------------------------------------------
void GdBrowserView::drop(int x, int y)
{
    BROWSER_DEBUG("drop at " << x << ", " << y);

    if (!m_browser || !m_browser->GetHost())
        return;

    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->DragTargetDrop(mouse_event);
    m_drag_data = nullptr;
}

//------------------------------------------------------------------------------
bool GdBrowserView::isDragging() const
{
    return m_is_dragging;
}

//------------------------------------------------------------------------------
bool GdBrowserView::onStartDragging(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefDragData> drag_data,
                                     CefRenderHandler::DragOperationsMask allowed_ops,
                                     int x,
                                     int y)
{
    BROWSER_DEBUG("onStartDragging at " << x << ", " << y);

    if (!m_drag_and_drop_enabled)
    {
        BROWSER_DEBUG("Drag and drop disabled, aborting drag");
        return false; // Abort the drag
    }

    // Store drag state
    m_is_dragging = true;
    m_drag_data = drag_data->Clone();
    m_drag_allowed_ops = allowed_ops;
    m_current_drag_op = DRAG_OPERATION_NONE;

    // Notify the browser that a drag is entering
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = m_mouse_event_modifiers;

    m_browser->GetHost()->DragTargetDragEnter(m_drag_data, mouse_event, allowed_ops);

    // Emit signal for Godot
    godot::Dictionary drag_info;
    if (drag_data->IsFile())
    {
        drag_info["type"] = "file";
        std::vector<CefString> file_names;
        drag_data->GetFileNames(file_names);
        godot::Array files;
        for (const auto& file : file_names)
        {
            files.push_back(godot::String(file.ToString().c_str()));
        }
        drag_info["files"] = files;
    }
    else if (drag_data->IsLink())
    {
        drag_info["type"] = "link";
        drag_info["url"] = godot::String(drag_data->GetLinkURL().ToString().c_str());
    }
    else if (drag_data->IsFragment())
    {
        drag_info["type"] = "fragment";
        drag_info["text"] = godot::String(drag_data->GetFragmentText().ToString().c_str());
        drag_info["html"] = godot::String(drag_data->GetFragmentHtml().ToString().c_str());
    }
    drag_info["x"] = x;
    drag_info["y"] = y;

    emit_signal("on_start_dragging", drag_info, this);

    return true; // Handle the drag operation
}

//------------------------------------------------------------------------------
void GdBrowserView::onUpdateDragCursor(CefRefPtr<CefBrowser> browser,
                                        CefRenderHandler::DragOperation operation)
{
    m_current_drag_op = operation;

    // Emit signal for Godot to update cursor if needed
    emit_signal("on_update_drag_cursor", static_cast<int>(operation), this);
}

//------------------------------------------------------------------------------
void GdBrowserView::onCursorChange(CefRefPtr<CefBrowser> browser,
                                    cef_cursor_type_t type)
{
    // Map CEF cursor types to Godot DisplayServer::CursorShape
    godot::DisplayServer::CursorShape godot_cursor =
        godot::DisplayServer::CURSOR_ARROW;

    switch (type)
    {
        case CT_POINTER:
            godot_cursor = godot::DisplayServer::CURSOR_ARROW;
            break;
        case CT_CROSS:
            godot_cursor = godot::DisplayServer::CURSOR_CROSS;
            break;
        case CT_HAND:
        case CT_GRAB:
            godot_cursor = godot::DisplayServer::CURSOR_POINTING_HAND;
            break;
        case CT_IBEAM:
            godot_cursor = godot::DisplayServer::CURSOR_IBEAM;
            break;
        case CT_WAIT:
            godot_cursor = godot::DisplayServer::CURSOR_WAIT;
            break;
        case CT_HELP:
            godot_cursor = godot::DisplayServer::CURSOR_HELP;
            break;
        case CT_PROGRESS:
            godot_cursor = godot::DisplayServer::CURSOR_BUSY;
            break;
        case CT_EASTRESIZE:
        case CT_WESTRESIZE:
        case CT_EASTWESTRESIZE:
        case CT_COLUMNRESIZE:
            godot_cursor = godot::DisplayServer::CURSOR_HSIZE;
            break;
        case CT_NORTHRESIZE:
        case CT_SOUTHRESIZE:
        case CT_NORTHSOUTHRESIZE:
        case CT_ROWRESIZE:
            godot_cursor = godot::DisplayServer::CURSOR_VSIZE;
            break;
        case CT_NORTHEASTRESIZE:
        case CT_SOUTHWESTRESIZE:
            godot_cursor = godot::DisplayServer::CURSOR_BDIAGSIZE;
            break;
        case CT_NORTHWESTRESIZE:
        case CT_SOUTHEASTRESIZE:
            godot_cursor = godot::DisplayServer::CURSOR_FDIAGSIZE;
            break;
        case CT_MOVE:
        case CT_GRABBING:
        case CT_MIDDLEPANNING:
            godot_cursor = godot::DisplayServer::CURSOR_MOVE;
            break;
        case CT_NODROP:
        case CT_NOTALLOWED:
            godot_cursor = godot::DisplayServer::CURSOR_FORBIDDEN;
            break;
        default:
            godot_cursor = godot::DisplayServer::CURSOR_ARROW;
            break;
    }

    // Only change cursor if it's different from the current one to avoid glitches
    if (m_current_cursor != godot_cursor)
    {
        // Store the current cursor
        m_current_cursor = godot_cursor;

        BROWSER_DEBUG("Changing cursor to " << int(godot_cursor));

        // Convert DisplayServer::CursorShape to Input::CursorShape (they have the same values)
        godot::Input::CursorShape input_cursor = static_cast<godot::Input::CursorShape>(godot_cursor);

        // Input::set_default_cursor_shape() only affects areas of the screen
        // NOT covered by a Control node. Since the CEF texture is usually
        // displayed inside a Control (e.g. TextureRect), that Control's own
        // "mouse_default_cursor_shape" wins on every mouse motion event and
        // silently overrides this call, making the cursor appear to never
        // change (or to flicker for a single frame). Kept here as a fallback
        // for setups where the texture is NOT displayed inside a Control
        // (e.g. a 3D mesh with no overlapping UI).
        godot::Input::get_singleton()->set_default_cursor_shape(input_cursor);

        // Apply the shape directly on the Control displaying our texture (set
        // by GdCEF::createBrowser() via setDisplayControl()). This is what
        // Godot actually uses while the mouse hovers that Control, so this is
        // the fix for the issue above: no GDScript wiring required.
        if (m_display_control != nullptr)
        {
            m_display_control->set_default_cursor_shape(
                static_cast<godot::Control::CursorShape>(int(godot_cursor)));
        }

        // Also notify GDScript, in case the application wants to react to
        // the cursor change itself (e.g. custom cursor rendering).
        emit_signal("on_cursor_changed", int(godot_cursor), this);
    }
}

//------------------------------------------------------------------------------
void GdBrowserView::endDragging(int x, int y)
{
    BROWSER_DEBUG("endDragging at " << x << ", " << y);

    if (!m_is_dragging || !m_browser || !m_browser->GetHost())
    {
        m_is_dragging = false;
        m_drag_data = nullptr;
        return;
    }

    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = m_mouse_event_modifiers;

    // Perform the drop
    if (m_current_drag_op != DRAG_OPERATION_NONE)
    {
        m_browser->GetHost()->DragTargetDrop(mouse_event);
    }
    else
    {
        m_browser->GetHost()->DragTargetDragLeave();
    }

    // Notify CEF that the drag has ended
    m_browser->GetHost()->DragSourceEndedAt(x, y, m_current_drag_op);
    m_browser->GetHost()->DragSourceSystemDragEnded();

    // Reset drag state
    m_is_dragging = false;
    m_drag_data = nullptr;
    m_current_drag_op = DRAG_OPERATION_NONE;
}
