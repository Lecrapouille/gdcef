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

#ifndef GDCEF_BROWSER_HPP
#define GDCEF_BROWSER_HPP

#if !defined(_WIN32)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#    pragma GCC diagnostic ignored "-Wparentheses"
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#    pragma GCC diagnostic ignored "-Wfloat-conversion"
#    pragma GCC diagnostic ignored "-Wfloat-equal"
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wshadow"
#    pragma GCC diagnostic ignored "-Wundef"
#    if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wcast-align"
#        pragma clang diagnostic ignored "-Wundef"
#        pragma clang diagnostic ignored "-Wshadow-field"
#        pragma clang diagnostic ignored "-Wcast-qual"
#    endif
#endif

#include "helper_files.hpp"

// Godot 4
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

// Chromium Embedded Framework
#include "cef_app.h"
#include "cef_client.h"
#include "cef_parser.h"
#include "cef_render_handler.h"
#include "wrapper/cef_helpers.h"

#include <array>
#include <chrono>
#include <iostream>

// ****************************************************************************
//! \brief Class wrapping the CefBrowser class and export methods for Godot
//! script. This class is instantiate by GDCef.
// ****************************************************************************
class GDBrowserView: public godot::Node
{
    friend class GDCef;

public: // Godot interfaces

    // -------------------------------------------------------------------------
    //! \brief Our initializer called by Godot.
    // -------------------------------------------------------------------------
    void _init();

    // -------------------------------------------------------------------------
    //! \brief Godot stuff
    // -------------------------------------------------------------------------
    GDCLASS(GDBrowserView, godot::Node);

protected:

    static void _bind_methods();

private: // CEF interfaces

    // *************************************************************************
    //! \brief Routing CEF audio to Godot streamer node.
    // *************************************************************************
    struct RoutingAudio
    {
        //! \brief Godot audio streamer
        godot::Ref<godot::AudioStreamGeneratorPlayback> streamer = nullptr;
        //! \brief Audio received from CEF
        godot::PackedVector2Array buffer;
        //! \brief Number of audio channels
        int channels = -1;
    };

    // *************************************************************************
    //! \brief Mandatory since Godot ref counter is conflicting with CEF ref
    //! counting and therefore we reach with pure virtual destructor called.
    //! To avoid this we have to create this intermediate class.
    // *************************************************************************
    class Impl: public CefClient,
                public CefRenderHandler,
                public CefLoadHandler,
                public CefAudioHandler,
                public CefLifeSpanHandler,
                public CefDownloadHandler
    {
    public:

        friend GDBrowserView;

        // ---------------------------------------------------------------------
        //! \brief Pass the owner instance.
        // ---------------------------------------------------------------------
        Impl(GDBrowserView& view) : m_owner(view) {}

        // ---------------------------------------------------------------------
        //! \brief Destructor
        // ---------------------------------------------------------------------
        virtual ~Impl();

    private: // CefClient::CefBaseRefCounted interfaces

        // ---------------------------------------------------------------------
        //! \brief CEF reference couting
        // ---------------------------------------------------------------------
        IMPLEMENT_REFCOUNTING(Impl);

    private: // CefClient interfaces

        // ---------------------------------------------------------------------
        //! \brief Return the handler for off-screen rendering events.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override
        {
            return this;
        }

        // ---------------------------------------------------------------------
        //! \brief Return the handler for browser load status events.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override
        {
            return this;
        }

        // ---------------------------------------------------------------------
        //! \brief Return the handler for audio rendering events.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefAudioHandler> GetAudioHandler() override
        {
            // FIXME this is called once, so we cannot swap modes :( How to do
            // that ?
            std::cout << (m_audio.streamer == nullptr
                              ? "GetAudioHandler CEF Audio"
                              : "GetAudioHandler Godot audio")
                      << "\n";
            return m_audio.streamer != nullptr ? this : nullptr;
        }

        // ---------------------------------------------------------------------
        //! \brief Return the handler for browser life span events.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
        {
            return this;
        }

        // ---------------------------------------------------------------------
        //! \brief Return the handler for download events.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() override
        {
            return this;
        }

        // ---------------------------------------------------------------------
        //! \brief Called when a message is received from a different process.
        // ---------------------------------------------------------------------
        virtual bool
        OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefProcessId source_process,
                                 CefRefPtr<CefProcessMessage> message) override
        {
            return m_owner.onProcessMessageReceived(
                browser, frame, source_process, message);
        }

    private: // CefRenderHandler interfaces

        // ---------------------------------------------------------------------
        //! \brief Get the view port.
        // ---------------------------------------------------------------------
        virtual void GetViewRect(CefRefPtr<CefBrowser> browser,
                                 CefRect& rect) override
        {
            m_owner.getViewRect(browser, rect);
        }

        // ---------------------------------------------------------------------
        //! \brief Called when an element should be painted. Pixel values passed
        //! to this method are scaled relative to view coordinates based on the
        //! value of CefScreenInfo.device_scale_factor returned from
        //! GetScreenInfo. |type| indicates whether the element is the view or
        //! the popup widget. |buffer| contains the pixel data for the whole
        //! image. |dirtyRects| contains the set of rectangles in pixel
        //! coordinates that need to be repainted. |buffer| will be
        //! |width|*|height|*4 bytes in size and represents a BGRA image with an
        //! upper-left origin. This method is only called when
        //! CefWindowInfo::shared_texture_enabled is set to false.
        // ---------------------------------------------------------------------
        virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                             CefRenderHandler::PaintElementType type,
                             const CefRenderHandler::RectList& dirtyRects,
                             const void* buffer,
                             int width,
                             int height) override
        {
            m_owner.onPaint(browser, type, dirtyRects, buffer, width, height);
        }

    private: // CefLoadHandler interfaces

        // ---------------------------------------------------------------------
        //! \brief Called when the browser is done loading a frame. The |frame|
        //! value will never be empty -- call the IsMain() method to check if
        //! this frame is the main frame. Multiple frames may be loading at the
        //! same time. Sub-frames may start or continue loading after the main
        //! frame load has ended. This method will not be called for same page
        //! navigations (fragments, history state, etc.) or for navigations that
        //! fail or are canceled before commit. For notification of overall
        //! browser load status use OnLoadingStateChange instead.
        // ---------------------------------------------------------------------
        virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               int httpStatusCode) override
        {
            m_owner.onLoadEnd(browser, frame, httpStatusCode);
        }

        // ---------------------------------------------------------------------
        //! \brief Called when a navigation fails or is canceled. This method
        //! may be called by itself if before commit or in combination with
        //! OnLoadStart/OnLoadEnd if after commit. |errorCode| is the error code
        //! number, |errorText| is the error text and |failedUrl| is the URL
        //! that failed to load. See net\base\net_error_list.h for complete
        //! descriptions of the error codes.
        // ---------------------------------------------------------------------
        virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 ErrorCode errorCode,
                                 const CefString& errorText,
                                 const CefString& failedUrl) override
        {
            m_owner.onLoadError(browser, frame, int(errorCode), errorText);
        }

    private: // CefAudioHandler interfaces

        virtual void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser,
                                          const CefAudioParameters& params,
                                          int channels) override
        {
            m_owner.onAudioStreamStarted(browser, params, channels);
        }

        virtual void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser,
                                         const float** data,
                                         int frames,
                                         int64_t pts) override
        {
            m_owner.onAudioStreamPacket(browser, data, frames, pts);
        }

        virtual void
        OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override
        {
        }

        virtual void OnAudioStreamError(CefRefPtr<CefBrowser> browser,
                                        const CefString& message) override
        {
        }

    private: // CefLifeSpanHandler interfaces

        virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   int popup_id,
                                   const CefString& target_url,
                                   const CefString& target_frame_name,
                                   WindowOpenDisposition target_disposition,
                                   bool user_gesture,
                                   const CefPopupFeatures& popupFeatures,
                                   CefWindowInfo& windowInfo,
                                   CefRefPtr<CefClient>& client,
                                   CefBrowserSettings& settings,
                                   CefRefPtr<CefDictionaryValue>& extra_info,
                                   bool* no_javascript_access) override
        {
            return m_owner.onBeforePopup(browser, target_url);
        }

    private: // CefDownloadHandler interfaces

        virtual bool CanDownload(CefRefPtr<CefBrowser> browser,
                                 const CefString& url,
                                 const CefString& request_method) override
        {
            return m_owner.canDownload(browser, url, request_method);
        }

        virtual bool
        OnBeforeDownload(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefDownloadItem> download_item,
                         const CefString& suggested_name,
                         CefRefPtr<CefBeforeDownloadCallback> callback) override
        {
            return m_owner.onBeforeDownload(
                browser, download_item, suggested_name, callback);
        }

        virtual void
        OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefDownloadItem> download_item,
                          CefRefPtr<CefDownloadItemCallback> callback) override
        {
            m_owner.onDownloadUpdated(browser, download_item, callback);
        }

    private:

        GDBrowserView& m_owner;
        RoutingAudio m_audio;
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Default Constructor. Initialize internal states. Nothing else is
    //! made because Godot engine will automatically call the _init() method.
    //! You shall complete the constructor by calling init(godot::String const&,
    //! CefBrowserSettings const&, CefWindowInfo const&) because Godot does not
    //! manage non dummy constructors.
    // -------------------------------------------------------------------------
    GDBrowserView();

    // -------------------------------------------------------------------------
    //! \brief Virtual to use dynamic_cast
    // -------------------------------------------------------------------------
    virtual ~GDBrowserView();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Return the globally unique
    //! identifier for this browser.  This value is also used as the tabId for
    //! extension APIs.
    //!
    //! \note Return -1 when the browser is not valid.
    // -------------------------------------------------------------------------
    inline int id() const
    {
        return m_id;
    }

    // -------------------------------------------------------------------------
    //! \brief Return the latest error.
    // -------------------------------------------------------------------------
    godot::String getError();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Return True if this object is
    //! currently valid. This will return false after
    //! CefLifeSpanHandler::OnBeforeClose is called.
    // -------------------------------------------------------------------------
    bool isValid() const;

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Set the render zoom level.
    // -------------------------------------------------------------------------
    void setZoomLevel(double delta);

    // -------------------------------------------------------------------------
    //! \brief Set if the browser can download files.
    // -------------------------------------------------------------------------
    void allowDownloads(bool allow);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Download the given file from
    //! URL.
    // -------------------------------------------------------------------------
    void downloadFile(godot::String url);

    // -------------------------------------------------------------------------
    //! \brief Set the path to the folder where to store downloaded files.
    // -------------------------------------------------------------------------
    void setDownloadFolder(godot::String folder);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Load the given web page from
    //! URL. \fixme Godot does not like String const& url why ?
    // -------------------------------------------------------------------------
    void loadURL(godot::String url);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Load the given web page from
    //! string content.
    // -------------------------------------------------------------------------
    void loadDataURI(godot::String html, godot::String mime_type);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Return true if a document has
    //! been loaded in the browser.
    // -------------------------------------------------------------------------
    bool loaded() const;

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Stop loading the page.
    // -------------------------------------------------------------------------
    void stopLoading();

    // -------------------------------------------------------------------------
    //! \brief Refresh the page.
    // -------------------------------------------------------------------------
    bool reload() const;

    // -------------------------------------------------------------------------
    //! \brief Execute copy the selected text in the clipboard.
    // -------------------------------------------------------------------------
    void copy() const;

    // -------------------------------------------------------------------------
    //! \brief Execute the paste from the clipboard content.
    //! FIXME https://github.com/chromiumembedded/cef/issues/3117
    // -------------------------------------------------------------------------
    void paste() const;

    // -------------------------------------------------------------------------
    //! \brief Execute cut the selected text.
    // -------------------------------------------------------------------------
    void cut() const;

    // -------------------------------------------------------------------------
    //! \brief Execute cut the selected text.
    // -------------------------------------------------------------------------
    void delete_() const;

    // -------------------------------------------------------------------------
    //! \brief Undo action.
    // -------------------------------------------------------------------------
    void undo() const;

    // -------------------------------------------------------------------------
    //! \brief Redo action.
    // -------------------------------------------------------------------------
    void redo() const;

    // -------------------------------------------------------------------------
    //! \brief Request the HTML content of the page. The result is given by the
    //! Godot callback
    // -------------------------------------------------------------------------
    void requestHtmlContent();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Execute  Execute a string of
    //  JavaScript code in this browser.
    // -------------------------------------------------------------------------
    void executeJavaScript(godot::String javascript);

    // -------------------------------------------------------------------------
    //! \brief Terminate. Memory is released.
    // -------------------------------------------------------------------------
    void close();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Get the current url of the
    //! browser.
    // -------------------------------------------------------------------------
    godot::String getURL() const;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    godot::String getTitle() const;

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Return the Godot texture holding
    //! the page content to other Godot element that needs it for the rendering
    //! (i.e. TextureRect: $TextureRect.texture = browser.get_texture()).
    //! \fixme FIXME Need mutex ?
    // -------------------------------------------------------------------------
    inline godot::Ref<godot::ImageTexture> getTexture()
    {
        return m_texture;
    }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline void setTexture(godot::Ref<godot::ImageTexture> t)
    {
        m_texture = t;
    }

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Return true if the browser can
    //! navigate to the previous page.
    // -------------------------------------------------------------------------
    bool canNavigateBackward() const;

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Navigate to the previous page
    //! if possible.
    // -------------------------------------------------------------------------
    void navigateBackward();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Return true if the browser can
    //! navigate to the next page.
    // -------------------------------------------------------------------------
    bool canNavigateForward() const;

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Navigate to the next page if
    //! possible.
    // -------------------------------------------------------------------------
    void navigateForward();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Set the new windows dimension.
    // -------------------------------------------------------------------------
    inline void resize(godot::Vector2 const& dim)
    {
        resize_(int(dim.x), int(dim.y));
    }

    // -------------------------------------------------------------------------
    //! \brief Set the viewport: the rectangle on the surface where to display
    //! the web document. Values are in percent of the dimension on the
    //! surface. If this function is not called default values are: x = y = 0
    //! and w = h = 1 meaning the whole surface will be mapped.
    //!
    //! \param[in] x, the ratio where the top left corner shall start [0 .. 1[.
    //! \param[in] y, the ratio where the top left corner shall start [0 .. 1[.
    //! \param[in] w, the ratio where the top left corner shall start ]0 .. 1].
    //! \param[in] h, the ratio where the top left corner shall start ]0 .. 1].
    //! \return false if arguments are incorrect.
    //!
    //! Example: viewport(0.0f, 0.0f, 1.0f, 1.0f) means the whole surface.
    //! Example: viewport(0.0f, 0.0f, 0.5f, 1.0f) means the left side of the
    //!   surface vertically split.
    //! Example: viewport(0.5f, 0.0f, 1.0f, 1.0f) means the right side of the
    //!   surface vertically split.
    // -------------------------------------------------------------------------
    bool viewport(float x, float y, float w, float h);

    // -------------------------------------------------------------------------
    //! \brief TODO
    // void executeJS(const std::string &cmd);
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Down then up on Left button
    // -------------------------------------------------------------------------
    void leftClick();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Down then up on Right button.
    // -------------------------------------------------------------------------
    void rightClick();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Down then up on middle button.
    // -------------------------------------------------------------------------
    void middleClick();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Left Mouse button up.
    // -------------------------------------------------------------------------
    void leftMouseUp();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Right Mouse button up.
    // -------------------------------------------------------------------------
    void rightMouseUp();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Middle Mouse button up.
    // -------------------------------------------------------------------------
    void middleMouseUp();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Left Mouse button down.
    // -------------------------------------------------------------------------
    void leftMouseDown();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Right Mouse button down.
    // -------------------------------------------------------------------------
    void rightMouseDown();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Middle Mouse button down.
    // -------------------------------------------------------------------------
    void middleMouseDown();

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Set the new mouse position.
    // -------------------------------------------------------------------------
    void mouseMove(int x, int y);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Mouse Wheel Vertical.
    // -------------------------------------------------------------------------
    void mouseWheelVertical(const int wDelta);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Mouse Wheel Horizontal.
    // -------------------------------------------------------------------------
    void mouseWheelHorizontal(const int wDelta);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Set the new keyboard state (char
    //! typed ...).
    // -------------------------------------------------------------------------
    void keyPress(int key, bool pressed, bool shift, bool alt, bool ctrl);

    //--------------------------------------------------------------------------
    //! \brief Mute or unmute the browser audio.
    //! \param[in] state set true for muting the audio else false to unmute.
    //! \return true if the audio has been muted.
    //--------------------------------------------------------------------------
    bool mute(bool state);

    //--------------------------------------------------------------------------
    //! \brief Return if the browser has its audio muted.
    //! \return true if the audio is muted.
    //--------------------------------------------------------------------------
    bool muted();

    // -------------------------------------------------------------------------
    //! \brief Set the audio streamer.
    // -------------------------------------------------------------------------
    void
    setAudioStreamer(godot::Ref<godot::AudioStreamGeneratorPlayback> streamer)
    {
        if (m_impl != nullptr)
        {
            m_impl->m_audio.streamer = streamer;
        }
    }

    // -------------------------------------------------------------------------
    //! \brief Get the audio streamer.
    // -------------------------------------------------------------------------
    godot::Ref<godot::AudioStreamGeneratorPlayback> getAudioStreamer()
    {
        if (m_impl == nullptr)
            return nullptr;
        return m_impl->m_audio.streamer;
    }

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Get the color of the currently
    //! hovered on pixel
    // -------------------------------------------------------------------------
    godot::Color getPixelColor(int x, int y) const;

    // -------------------------------------------------------------------------
    //! \brief Register a GDScript method in the JavaScript context.
    //! The registered GDScript method can be called from JavaScript using:
    //!     window.godot.methodName(args)
    //!
    //! \param[in] callable the GDScript method to register. The callable must
    //! be  a valid GDScript method that can receive string parameters.
    //!
    //! \return true if the method has been successfully registered, false
    //! otherwise.
    //!
    //! \note The registered method will be available in JavaScript under the
    //! 'window.godot' namespace. All parameters passed from JavaScript will
    //! be converted to a Godot::Variant before been executed.
    //!
    //! Example in GDScript:
    //!     func my_method(data: String) -> void:
    //!         print("Received from JS: ", data)
    //!
    //!     browser.register_method(Callable(self, "my_method"))
    //!
    //! Example in JavaScript:
    //!     window.godot.my_method("Hello from JS!");
    //!
    // -------------------------------------------------------------------------
    bool registerGodotMethod(const godot::Callable& callable);

    // -------------------------------------------------------------------------
    //! \brief Send a message to the JavaScript side.
    //! The message will be received in JavaScript as a JSON object.
    //!
    //! \param[in] eventName Name of the event to trigger in JavaScript.
    //! \param[in] data Godot::Variant to send.
    //! \return true if the message has been sent, false otherwise.
    //!
    //! Example in GDScript:
    //!     browser.sendToJS("myEvent", {"key": "value"})
    //!
    //! Example in JavaScript:
    //!     window.addEventListener("myEvent", function(event) {
    //!         const data = JSON.parse(event.data);
    //!         console.log(data.key); // outputs: "value"
    //!     });
    // -------------------------------------------------------------------------
    bool sendToJS(godot::String eventName, const godot::Variant& data);

private:

    void resize_(int width, int height);

    // -------------------------------------------------------------------------
    //! \brief hack: since Godot does not like Constructor with parameters we
    //! have to finalize GDBrowserView::GDBrowserView().
    //! \return the browser unique identifier or -1 in case of failure.
    // -------------------------------------------------------------------------
    int init(godot::String const& url,
             CefBrowserSettings const& cef_settings,
             CefWindowInfo const& window_info);

    // -------------------------------------------------------------------------
    //! \brief Called by GDBrowserView::Impl::GetViewRect
    // -------------------------------------------------------------------------
    void getViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);

    // -------------------------------------------------------------------------
    //! \brief Called by GDBrowserView::Impl::OnPaint
    // -------------------------------------------------------------------------
    void onPaint(CefRefPtr<CefBrowser> browser,
                 CefRenderHandler::PaintElementType type,
                 const CefRenderHandler::RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height);

    // -------------------------------------------------------------------------
    //! \brief Called by GDBrowserView::Impl::OnLoadEnd
    // -------------------------------------------------------------------------
    void onLoadEnd(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   int httpStatusCode);

    // -------------------------------------------------------------------------
    //! \brief Called by GDBrowserView::Impl::OnLoadError
    // -------------------------------------------------------------------------
    void onLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     const int errCode,
                     const CefString& errorText);

    // -------------------------------------------------------------------------
    //! \brief Called on a browser audio capture thread when the browser starts
    //! streaming audio. OnAudioStreamStopped will always be called after
    //! OnAudioStreamStarted; both methods may be called multiple times
    //! for the same browser. |params| contains the audio parameters like
    //! sample rate and channel layout. |channels| is the number of channels.
    // -------------------------------------------------------------------------
    void onAudioStreamStarted(CefRefPtr<CefBrowser> browser,
                              const CefAudioParameters& params,
                              int channels);

    // -------------------------------------------------------------------------
    //! \brief Called on the audio stream thread when a PCM packet is received
    //! for the stream. |data| is an array representing the raw PCM data as a
    //! floating point type, i.e. 4-byte value(s). |frames| is the number of
    //! frames in the PCM packet. |pts| is the presentation timestamp (in
    //! milliseconds since the Unix Epoch) and represents the time at which the
    //! decompressed packet should be presented to the user. Based on |frames|
    //! and the |channel_layout| value passed to OnAudioStreamStarted you can
    //! calculate the size of the |data| array in bytes.
    // -------------------------------------------------------------------------
    void onAudioStreamPacket(CefRefPtr<CefBrowser> browser,
                             const float** data,
                             int frames,
                             int64_t pts);

    // -------------------------------------------------------------------------
    //! \brief Called to prevent opening page on new windows.
    //! \return true to cancel the creation of the popup.
    //! Example:
    //! <button onclick="window.open('/start', '_blank');">Start</button>
    // -------------------------------------------------------------------------
    bool onBeforePopup(CefRefPtr<CefBrowser> browser,
                       const CefString& target_url);

    // -------------------------------------------------------------------------
    //! \brief Called before a download begins.
    //! \return true to allow the download.
    // -------------------------------------------------------------------------
    bool canDownload(CefRefPtr<CefBrowser> browser,
                     const CefString& url,
                     const CefString& request_method);

    // -------------------------------------------------------------------------
    //! \brief Called before a download begins.
    // -------------------------------------------------------------------------
    bool onBeforeDownload(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefDownloadItem> download_item,
                          const CefString& suggested_name,
                          CefRefPtr<CefBeforeDownloadCallback> callback);

    // -------------------------------------------------------------------------
    //! \brief Called when a download is updated.
    // -------------------------------------------------------------------------
    void onDownloadUpdated(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefDownloadItem> download_item,
                           CefRefPtr<CefDownloadItemCallback> callback);

    // -------------------------------------------------------------------------
    //! \brief Called when a message is received from a different process.
    // -------------------------------------------------------------------------
    bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message);

private:

    //! \brief CEF interface implementation
    friend GDBrowserView::Impl;

    //! \brief CEF interface implementation
    CefRefPtr<Impl> m_impl = nullptr;

    //! \brief One to one CEF browser. The GDCef is the class containing the
    //! whole browsers.
    CefRefPtr<CefBrowser> m_browser = nullptr;

    //! \brief Godot's temporary image (CEF => Godot)
    godot::Ref<godot::ImageTexture> m_texture;
    godot::Ref<godot::Image> m_image;
    godot::PackedByteArray m_data;

    //! \brief Mouse cursor position on the main window
    int m_mouse_x = 0;
    int m_mouse_y = 0;

    //! \brief Mouse button modifiers on the mouse event
    uint32_t m_mouse_event_modifiers = 0;

    //! \brief Left mouse button click counting for double-click and more
    int m_left_click_count = 1;
    std::chrono::system_clock::time_point m_last_left_down;

    //! \brief Browser's view dimension.
    //! Initial browser's view size. We expose it to Godot which can set the
    //! desired size depending on its viewport size.
    float m_width = 128.0f;
    float m_height = 128.0f;

    //! \brief The reagion in where to paint the CEF texture on the Godot
    //! surface.
    std::array<float, 4> m_viewport;

    //! \brief Cache unique identifier
    int m_id = -1;

    //! \brief Hold last error messages
    mutable std::stringstream m_error;

    //! \brief Allow downloads (configured from Browser config)
    bool m_allow_downloads = true;

    //! \brief Download folder (configured from Browser config)
    fs::path m_download_folder;

    //! \brief
    std::unordered_map<std::string, godot::Callable> m_js_bindings;
};

#if !defined(_WIN32)
#    if defined(__clang__)
#        pragma clang diagnostic pop
#    endif
#    pragma GCC diagnostic pop
#endif

#endif // GDCEF_BROWSER_HPP
