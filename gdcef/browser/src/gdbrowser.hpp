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
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "cef_app.h"
#include "cef_client.h"
#include "cef_display_handler.h"
#include "cef_drag_handler.h"
#include "cef_parser.h"
#include "cef_render_handler.h"
#include "wrapper/cef_helpers.h"

#include <array>
#include <chrono>
#include <iostream>

#include "ad_blocker.hpp"

// ****************************************************************************
//! \brief Class wrapping the CefBrowser class and export methods for Godot
//! script. This class is instantiate by GdCEF.
// ****************************************************************************
class GdBrowserView: public godot::Node
{
    friend class GdCEF;

public: // Godot interfaces

    // -------------------------------------------------------------------------
    //! \brief Our initializer called by Godot.
    // -------------------------------------------------------------------------
    void _init();

    // -------------------------------------------------------------------------
    //! \brief Godot stuff
    // -------------------------------------------------------------------------
    GDCLASS(GdBrowserView, godot::Node);

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
                public CefDownloadHandler,
                public CefRequestHandler,
                public CefResourceRequestHandler,
                public CefDragHandler,
                public CefDisplayHandler
    {
    public:

        friend GdBrowserView;

        // ---------------------------------------------------------------------
        //! \brief Pass the owner instance.
        // ---------------------------------------------------------------------
        Impl(GdBrowserView& view) : m_owner(view)
        {
            m_ad_blocker = new AdBlocker();
            assert((m_ad_blocker != nullptr) && "Failed allocating AdBlocker");
        }

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
        //! \brief Return the handler for request filtering.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override
        {
            return this;
        }

        // ---------------------------------------------------------------------
        //! \brief Return the handler for drag events.
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefDragHandler> GetDragHandler() override
        {
            return this;
        }

        // ---------------------------------------------------------------------
        //! \brief Return the handler for display events (cursor changes, etc.).
        // ---------------------------------------------------------------------
        virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
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
        //! \brief Called when the browser wants to show or hide the popup
        //! widget (i.e. an expanded <select> list).
        // ---------------------------------------------------------------------
        virtual void OnPopupShow(CefRefPtr<CefBrowser> browser,
                                 bool show) override
        {
            m_owner.onPopupShow(browser, show);
        }

        // ---------------------------------------------------------------------
        //! \brief Called when the browser wants to move or resize the popup
        //! widget. \param[in] rect the new location and size in view
        //! coordinates.
        // ---------------------------------------------------------------------
        virtual void OnPopupSize(CefRefPtr<CefBrowser> browser,
                                 const CefRect& rect) override
        {
            m_owner.onPopupSize(browser, rect);
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

        // ---------------------------------------------------------------------
        //! \brief Called when the user starts dragging content in the web view.
        //! Contextual information about the dragged content is supplied by
        //! |drag_data|. (|x|, |y|) is the drag start location in screen
        //! coordinates. OS APIs that run a system message loop may be used
        //! within the StartDragging call. Return false to abort the drag
        //! operation. Don't call any of CefBrowserHost::DragSource*Ended*
        //! methods after returning false. Return true to handle the drag
        //! operation. Call CefBrowserHost::DragSourceEndedAt and
        //! DragSourceSystemDragEnded either synchronously or asynchronously to
        //! inform CEF that the drag operation has ended.
        // ---------------------------------------------------------------------
        virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefDragData> drag_data,
                                   CefRenderHandler::DragOperationsMask allowed_ops,
                                   int x,
                                   int y) override
        {
            return m_owner.onStartDragging(browser, drag_data, allowed_ops, x, y);
        }

        // ---------------------------------------------------------------------
        //! \brief Called when the web view wants to update the mouse cursor
        //! during a drag & drop operation. |operation| describes the allowed
        //! operation (none, move, copy, link).
        // ---------------------------------------------------------------------
        virtual void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
                                      CefRenderHandler::DragOperation operation) override
        {
            m_owner.onUpdateDragCursor(browser, operation);
        }

    private: // CefLoadHandler interfaces

        // ---------------------------------------------------------------------
        //! \brief Called when the browser begins loading a frame. The |frame|
        //! value will never be empty -- call the IsMain() method to check if
        //! this frame is the main frame. Multiple frames may be loading at the
        //! same time. Sub-frames may start or continue loading after the main
        //! frame load has ended. This method may not be called for a particular
        //! frame if the load request for that frame fails.
        // ---------------------------------------------------------------------
        virtual void OnLoadStart(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 TransitionType transition_type) override
        {
            m_owner.onLoadStart(browser, frame);
        }

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

        virtual bool OnBeforePopup(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            int popup_id,
            const CefString& target_url,
            const CefString& target_frame_name,
            CefLifeSpanHandler::WindowOpenDisposition target_disposition,
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

    private: // CefDragHandler interfaces

        // ---------------------------------------------------------------------
        //! \brief Called when an external drag event enters the browser window.
        //! |dragData| contains the drag event data and |mask| represents the
        //! type of drag operation. Return false for default drag handling
        //! behavior or true to cancel the drag event.
        // ---------------------------------------------------------------------
        virtual bool OnDragEnter(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefDragData> dragData,
                                 CefDragHandler::DragOperationsMask mask) override
        {
            return m_owner.onDragEnter(browser, dragData, mask);
        }

        // ---------------------------------------------------------------------
        //! \brief Called whenever draggable regions for the browser window
        //! change. These can be specified using the '-webkit-app-region: drag/
        //! no-drag' CSS-property. If draggable regions are never defined in a
        //! document this method will also never be called. If the last
        //! draggable region is removed from a document this method will be
        //! called with an empty vector.
        // ---------------------------------------------------------------------
        virtual void
        OnDraggableRegionsChanged(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  const std::vector<CefDraggableRegion>& regions) override
        {
            m_owner.onDraggableRegionsChanged(browser, frame, regions);
        }

    private: // CefDisplayHandler interfaces

        // ---------------------------------------------------------------------
        //! \brief Called when the browser's cursor has changed. If |type| is
        //! CT_CUSTOM then |custom_cursor_info| will be populated with the custom
        //! cursor information. Return true if the cursor change was handled or
        //! false for default handling.
        // ---------------------------------------------------------------------
        virtual bool OnCursorChange(CefRefPtr<CefBrowser> browser,
                                    CefCursorHandle cursor,
                                    cef_cursor_type_t type,
                                    const CefCursorInfo& custom_cursor_info) override
        {
            m_owner.onCursorChange(browser, type);
            return true;
        }

    private: // CefRequestContextHandler interfaces

        virtual CefResourceRequestHandler::ReturnValue
        OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefRefPtr<CefRequest> request,
                             CefRefPtr<CefCallback> callback) override
        {
            return m_ad_blocker->OnBeforeResourceLoad(
                browser, frame, request, callback);
        }

        virtual CefRefPtr<CefResourceRequestHandler>
        GetResourceRequestHandler(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefRequest> request,
                                  bool is_navigation,
                                  bool is_download,
                                  const CefString& request_initiator,
                                  bool& disable_default_handling) override
        {
            return m_ad_blocker;
        }

    private:

        GdBrowserView& m_owner;
        CefRefPtr<AdBlocker> m_ad_blocker;
        RoutingAudio m_audio;
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Default Constructor. Initialize internal states. Nothing else is
    //! made because Godot engine will automatically call the _init() method.
    //! You shall complete the constructor by calling init(godot::String const&,
    //! CefBrowserSettings const&, CefWindowInfo const&) because Godot does not
    //! manage non dummy constructors.
    //!
    //! \note The CEF client and the Godot texture are allocated by
    //! init() and not here, because Godot also builds throw-away instances of
    //! registered classes (to collect the default value of the properties or
    //! to generate the class documentation) for which this is pure waste.
    // -------------------------------------------------------------------------
    GdBrowserView();

    // -------------------------------------------------------------------------
    //! \brief Virtual to use dynamic_cast
    // -------------------------------------------------------------------------
    virtual ~GdBrowserView();

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
    //! Godot signal on_html_content_requested.
    // -------------------------------------------------------------------------
    void requestHtmlContent();

    // -------------------------------------------------------------------------
    //! \brief Save the current page HTML content to a file.
    //! The result is given by the Godot signal on_page_saved.
    //! \param[in] path The file path to save to. Supports Godot paths
    //!            (res://, user://) and absolute paths.
    // -------------------------------------------------------------------------
    void savePage(godot::String path);

    // -------------------------------------------------------------------------
    //! \brief Save the current page as a PDF file with all rendered content
    //! (images, CSS, etc.). The result is given by the Godot signal on_pdf_saved.
    //! \param[in] path The file path to save to (must end with .pdf).
    //!            Supports Godot paths (res://, user://) and absolute paths.
    // -------------------------------------------------------------------------
    void savePageAsPdf(godot::String path);

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
    //! \return a null reference as long as the browser has not been initialized
    //! by GdCEF::createBrowser().
    //!
    //! \note No mutex is needed to share this texture with CEF: since
    //! CefSettings::multi_threaded_message_loop is disabled and the CEF message
    //! loop is pumped by GdCEF::_process() calling CefDoMessageLoopWork(), the
    //! CEF UI thread filling this texture in onPaint() is the Godot main
    //! thread. The only callback running on another thread is
    //! onAudioStreamPacket().
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
    //! \brief Set the Control node (typically the TextureRect) displaying
    //! this browser's texture, so the mouse cursor requested by the page can
    //! be applied on it directly (see onCursorChange()). Called by
    //! GdCEF::createBrowser(); you should not need to call this yourself.
    //!
    //! \note If several browsers (e.g. tabs) share the same Control (only
    //! one visible/receiving mouse events at a time), the last browser to
    //! change its cursor wins on that Control. This is harmless as long as
    //! only the browser currently receiving mouse events can trigger a
    //! cursor change, which is the case if your application only forwards
    //! mouse events (setMouseMoved()) to the currently displayed browser.
    // -------------------------------------------------------------------------
    inline void setDisplayControl(godot::Control* control)
    {
        m_display_control = control;
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
    //! \param[in] wDelta The scroll delta (positive = up, negative = down).
    //! \param[in] shift True if Shift key is pressed.
    //! \param[in] ctrl True if Ctrl key is pressed.
    //! \param[in] alt True if Alt key is pressed.
    // -------------------------------------------------------------------------
    void mouseWheelVertical(int wDelta, bool shift = false, bool ctrl = false,
                            bool alt = false);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Mouse Wheel Horizontal.
    //! \param[in] wDelta The scroll delta (positive = right, negative = left).
    //! \param[in] shift True if Shift key is pressed.
    //! \param[in] ctrl True if Ctrl key is pressed.
    //! \param[in] alt True if Alt key is pressed.
    // -------------------------------------------------------------------------
    void mouseWheelHorizontal(int wDelta, bool shift = false, bool ctrl = false,
                              bool alt = false);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Touch point pressed. Allows
    //! multi-touch: each finger uses its own id (CEF tracks up to 16).
    //! \param[in] id Unique id of the touch point (one id per finger).
    //! \param[in] x The touch x position.
    //! \param[in] y The touch y position.
    // -------------------------------------------------------------------------
    void touchDown(int id, int x, int y);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Touch point moved.
    //! \param[in] id Unique id of the touch point (one id per finger).
    //! \param[in] x The touch x position.
    //! \param[in] y The touch y position.
    // -------------------------------------------------------------------------
    void touchMove(int id, int x, int y);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Touch point released.
    //! \param[in] id Unique id of the touch point (one id per finger).
    //! \param[in] x The touch x position.
    //! \param[in] y The touch y position.
    // -------------------------------------------------------------------------
    void touchUp(int id, int x, int y);

    // -------------------------------------------------------------------------
    //! \brief Exported method to Godot script. Touch point cancelled (e.g.
    //! palm rejection or focus loss).
    //! \param[in] id Unique id of the touch point (one id per finger).
    // -------------------------------------------------------------------------
    void touchCancel(int id);

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
    //!     window.godotMethods.methodName(args)
    //!
    //! \param[in] object The object to register the method from.
    //! \param[in] method_name The name of the method to register.
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
    //!     browser.register_method(self, "my_method")
    //!
    //! Example in JavaScript:
    //!     window.godotMethods.my_method("Hello from JS!");
    //!
    // -------------------------------------------------------------------------
    bool registerGodotMethod(godot::Object* object, godot::String method_name);

    // -------------------------------------------------------------------------
    //! \brief Send a message to the JavaScript side.
    //! The message will be received in JavaScript as a JSON object.
    //!
    //! \param[in] event_name Name of the event to trigger in JavaScript.
    //! \param[in] data Godot::Variant to send.
    //! \return true if the message has been sent, false otherwise.
    //!
    //! Example in GDScript:
    //!     browser.js_emit("myEvent", {"key": "value"})
    //!
    //! Example in JavaScript:
    //!     window.godotEvents.on("myEvent", function(event) {
    //!         const data = JSON.parse(event.data);
    //!         console.log(data.key); // outputs: "value"
    //!     });
    // -------------------------------------------------------------------------
    bool jsEmit(godot::String event_name, const godot::Variant& data);

    // -------------------------------------------------------------------------
    //! \brief Add a custom pattern to the ad blocker
    //! \param[in] pattern Rule in EasyList format ("||domain.com^",
    //! "/path/pattern", "domain.com", "keyword", prefixed by "@@" to whitelist)
    //! \return true if pattern was successfully added
    // -------------------------------------------------------------------------
    bool addAdBlockPattern(godot::String pattern);

    // -------------------------------------------------------------------------
    //! \brief Add to the ad blocker all the rules of an EasyList compatible
    //! filter list file (one rule per line, "!" and "[" starting a comment).
    //! \param[in] filepath Path of the filter list ("res://", "user://" and
    //! system paths are accepted).
    //! \return The number of rules that have been added.
    // -------------------------------------------------------------------------
    int loadAdBlockFilterList(godot::String filepath);

    // -------------------------------------------------------------------------
    //! \brief Remove all the ad blocker rules, including the default ones, to
    //! start from an empty filter list.
    // -------------------------------------------------------------------------
    void clearAdBlockRules();

    // -------------------------------------------------------------------------
    //! \brief Get the number of rules currently held by the ad blocker.
    //! \return A human readable description of the loaded rules.
    // -------------------------------------------------------------------------
    godot::String getAdBlockStats() const;

    // -------------------------------------------------------------------------
    //! \brief Enable or disable ad blocking
    //! \param[in] enable True to enable ad blocking, false to disable
    // -------------------------------------------------------------------------
    void enableAdBlock(bool enable);

    // -------------------------------------------------------------------------
    //! \brief Check if ad blocking is enabled
    //! \return True if ad blocking is enabled
    // -------------------------------------------------------------------------
    bool isAdBlockEnabled() const;

    // -------------------------------------------------------------------------
    //! \brief Write an info message to CEF logs from GDScript.
    //! \param[in] message the info message to log
    // -------------------------------------------------------------------------
    void log_info(godot::String message);

    // -------------------------------------------------------------------------
    //! \brief Write a warning message to CEF logs from GDScript.
    //! \param[in] message the warning message to log
    // -------------------------------------------------------------------------
    void log_warning(godot::String message);

    // -------------------------------------------------------------------------
    //! \brief Write an error message to CEF logs from GDScript.
    //! \param[in] message the error message to log
    // -------------------------------------------------------------------------
    void log_error(godot::String message);

    // -------------------------------------------------------------------------
    //! \brief Write a fatal message to CEF logs from GDScript. Note: written
    //! with the error severity, this does not terminate the application.
    //! \param[in] message the fatal message to log
    // -------------------------------------------------------------------------
    void log_fatal(godot::String message);

    // -------------------------------------------------------------------------
    //! \brief Enable or disable drag and drop handling.
    //! \param[in] enable True to enable drag and drop, false to disable.
    //! When disabled, all drag enter events are cancelled.
    // -------------------------------------------------------------------------
    void enableDragAndDrop(bool enable);

    // -------------------------------------------------------------------------
    //! \brief Check if drag and drop is enabled.
    //! \return True if drag and drop is enabled.
    // -------------------------------------------------------------------------
    bool isDragAndDropEnabled() const;

    // -------------------------------------------------------------------------
    //! \brief Notify the browser that a drag operation has entered.
    //! Call this when a Godot drag event starts over the browser.
    //! \param[in] x The x position of the drag event.
    //! \param[in] y The y position of the drag event.
    //! \param[in] text Optional text data being dragged.
    //! \param[in] html Optional HTML data being dragged.
    //! \param[in] url Optional URL being dragged.
    // -------------------------------------------------------------------------
    void dragEnter(int x, int y, godot::String text, godot::String html,
                   godot::String url);

    // -------------------------------------------------------------------------
    //! \brief Notify the browser that a drag operation is moving over.
    //! Call this when a Godot drag event moves over the browser.
    //! \param[in] x The x position of the drag event.
    //! \param[in] y The y position of the drag event.
    // -------------------------------------------------------------------------
    void dragOver(int x, int y);

    // -------------------------------------------------------------------------
    //! \brief Notify the browser that a drag operation has left the window.
    // -------------------------------------------------------------------------
    void dragLeave();

    // -------------------------------------------------------------------------
    //! \brief Notify the browser that a drop has occurred.
    //! \param[in] x The x position of the drop event.
    //! \param[in] y The y position of the drop event.
    // -------------------------------------------------------------------------
    void drop(int x, int y);

    // -------------------------------------------------------------------------
    //! \brief Check if an internal HTML5 drag operation is currently in progress.
    //! \return True if dragging is in progress.
    // -------------------------------------------------------------------------
    bool isDragging() const;

    // -------------------------------------------------------------------------
    //! \brief End the current internal HTML5 drag operation.
    //! Call this when the mouse button is released during a drag.
    //! \param[in] x The x position where the drag ended.
    //! \param[in] y The y position where the drag ended.
    // -------------------------------------------------------------------------
    void endDragging(int x, int y);

private:

    void resize_(int width, int height);

    // -------------------------------------------------------------------------
    //! \brief hack: since Godot does not like Constructor with parameters we
    //! have to finalize GdBrowserView::GdBrowserView().
    //! \return the browser unique identifier or -1 in case of failure.
    // -------------------------------------------------------------------------
    int init(godot::String const& url,
             CefBrowserSettings const& cef_settings,
             CefWindowInfo const& window_info);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::GetViewRect
    // -------------------------------------------------------------------------
    void getViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnPaint
    // -------------------------------------------------------------------------
    void onPaint(CefRefPtr<CefBrowser> browser,
                 CefRenderHandler::PaintElementType type,
                 const CefRenderHandler::RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height);

    // -------------------------------------------------------------------------
    //! \brief Called by onPaint() for the pixels of the popup widget, which CEF
    //! renders in its own buffer.
    // -------------------------------------------------------------------------
    void onPaintPopup(const void* buffer, int width, int height);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnPopupShow
    // -------------------------------------------------------------------------
    void onPopupShow(CefRefPtr<CefBrowser> browser, bool show);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnPopupSize
    // -------------------------------------------------------------------------
    void onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect);

    // -------------------------------------------------------------------------
    //! \brief Composite the pixels of the popup widget over the page pixels.
    //! Done again after each page paint since the page erases the popup.
    // -------------------------------------------------------------------------
    void compositePopup();

    // -------------------------------------------------------------------------
    //! \brief Send the page pixels to the Godot texture.
    //! \param[in] recreate set to true when the dimension changed, since
    //! ImageTexture::update() only accepts an image of the same dimension.
    // -------------------------------------------------------------------------
    void updateTexture(bool recreate);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnLoadStart
    // -------------------------------------------------------------------------
    void onLoadStart(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnLoadEnd
    // -------------------------------------------------------------------------
    void onLoadEnd(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   int httpStatusCode);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnLoadError
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
    //! \brief Called when an IPC message is received from the render process.
    //! Execute the requested Godot method.
    // -------------------------------------------------------------------------
    bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnDragEnter
    //! \return true to cancel the drag event, false for default handling.
    // -------------------------------------------------------------------------
    bool onDragEnter(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefDragData> dragData,
                     CefDragHandler::DragOperationsMask mask);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnDraggableRegionsChanged
    // -------------------------------------------------------------------------
    void onDraggableRegionsChanged(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   const std::vector<CefDraggableRegion>& regions);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::StartDragging when the user starts
    //! dragging content in the web view (HTML5 drag and drop).
    //! \return true to handle the drag operation, false to abort.
    // -------------------------------------------------------------------------
    bool onStartDragging(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefDragData> drag_data,
                         CefRenderHandler::DragOperationsMask allowed_ops,
                         int x,
                         int y);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::UpdateDragCursor
    // -------------------------------------------------------------------------
    void onUpdateDragCursor(CefRefPtr<CefBrowser> browser,
                            CefRenderHandler::DragOperation operation);

    // -------------------------------------------------------------------------
    //! \brief Called by GdBrowserView::Impl::OnCursorChange when the browser
    //! cursor changes (e.g., pointer, hand, text, etc.)
    // -------------------------------------------------------------------------
    void onCursorChange(CefRefPtr<CefBrowser> browser, cef_cursor_type_t type);

    // -------------------------------------------------------------------------
    //! \brief Display the given mouse cursor shape and emit the
    //! "on_cursor_changed" signal. Does nothing when the shape is already the
    //! displayed one.
    //! \param[in] shape the shape asked by the web page.
    // -------------------------------------------------------------------------
    void applyCursorShape(godot::DisplayServer::CursorShape shape);

    // -------------------------------------------------------------------------
    //! \brief Recursively convert JSON data to Godot Variant types
    //! \param[in] json JSON value to convert
    //! \return Converted Godot Variant
    // -------------------------------------------------------------------------
    godot::Variant JsonToGodot(const godot::Dictionary& json);

    // -------------------------------------------------------------------------
    //! \brief Recursively convert JSON array to Godot Variant types
    //! \param[in] json_array JSON array to convert
    //! \return Converted Godot Array
    // -------------------------------------------------------------------------
    godot::Variant JsonToGodot(const godot::Array& json_array);

    // -------------------------------------------------------------------------
    //! \brief Generic converter for any JSON value to Godot Variant types
    //! \param[in] json_value JSON value to convert
    //! \return Converted Godot Variant
    // -------------------------------------------------------------------------
    godot::Variant JsonToGodot(const godot::Variant& json_value);

private:

    //! \brief CEF interface implementation
    friend GdBrowserView::Impl;

    //! \brief CEF interface implementation
    CefRefPtr<Impl> m_impl = nullptr;

    //! \brief One to one CEF browser. The GdCEF is the class containing the
    //! whole browsers.
    CefRefPtr<CefBrowser> m_browser = nullptr;

    //! \brief Godot's texture holding the page content (CEF => Godot)
    godot::Ref<godot::ImageTexture> m_texture;

    //! \brief Page pixels converted from CEF's BGRA to Godot's RGBA. Kept
    //! between paints so that CEF's dirty rectangles can be applied on the
    //! previous frame instead of converting the whole page.
    godot::PackedByteArray m_data;

    //! \brief Dimension of the last painted CEF buffer, and therefore the
    //! dimension of the pixels currently held by m_data and m_texture. Differs
    //! from m_width and m_height, which are the desired dimension that CEF only
    //! applies at its next paint.
    int m_painted_width = 0;
    int m_painted_height = 0;

    //! \brief Pixels of the popup widget (i.e. an expanded <select> list),
    //! converted from CEF's BGRA to Godot's RGBA. CEF renders the popup in its
    //! own buffer, so they are kept to be composited over the page again each
    //! time the page below is repainted. Empty when no popup is shown.
    godot::PackedByteArray m_popup_data;
    int m_popup_width = 0;
    int m_popup_height = 0;

    //! \brief Position of the popup widget inside the page, in pixels, given by
    //! CEF and clamped to stay inside the page.
    int m_popup_x = 0;
    int m_popup_y = 0;

    //! \brief Set when the pixels we hold are not those of the page (a popup
    //! widget was composited over them) and CEF's dirty rectangles are
    //! therefore not enough to refresh them.
    bool m_repaint_page = false;

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

    //! \brief JS bindings
    std::unordered_map<std::string, godot::Callable> m_js_bindings;

    //! \brief Enable or disable drag and drop. When disabled, all drag enter
    //! events are cancelled.
    bool m_drag_and_drop_enabled = true;

    //! \brief Current drag data (used for drag operations from Godot to CEF)
    CefRefPtr<CefDragData> m_drag_data = nullptr;

    //! \brief True when an internal HTML5 drag operation is in progress
    bool m_is_dragging = false;

    //! \brief Allowed drag operations for the current drag
    CefRenderHandler::DragOperationsMask m_drag_allowed_ops = DRAG_OPERATION_NONE;

    //! \brief Current drag operation
    CefRenderHandler::DragOperation m_current_drag_op = DRAG_OPERATION_NONE;

    //! \brief Current cursor shape (stored to reapply after Godot resets it)
    godot::DisplayServer::CursorShape m_current_cursor = godot::DisplayServer::CURSOR_ARROW;

    //! \brief Cursor shape displayed when the drag started, restored once the
    //! drag ends: while dragging, CEF replaces OnCursorChange() by
    //! UpdateDragCursor() and therefore never asks for the previous shape back.
    godot::DisplayServer::CursorShape m_cursor_before_drag = godot::DisplayServer::CURSOR_ARROW;

    //! \brief The Control node displaying this browser's texture (usually a
    //! TextureRect, set by GdCEF::createBrowser()). Used by onCursorChange()
    //! to apply the cursor shape directly on it, since this Control's own
    //! "mouse_default_cursor_shape" is what Godot actually displays whenever
    //! the mouse hovers it. Not owned: assumed to outlive this browser.
    godot::Control* m_display_control = nullptr;
};

#if !defined(_WIN32)
#    if defined(__clang__)
#        pragma clang diagnostic pop
#    endif
#    pragma GCC diagnostic pop
#endif

#endif // GDCEF_BROWSER_HPP
