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
#include "gdcef.hpp"
#include "helper.hpp"
#include <iostream>

//------------------------------------------------------------------------------
// List of file names
#if defined(_WIN32)
#  define SUBPROCESS_NAME "gdcefSubProcess.exe"
#  define NEEDED_LIBRARIES "libcef.dll", "libgdcef.dll", "vulkan-1.dll", \
        "vk_swiftshader.dll", "libGLESv2.dll", "libEGL.dll"
#elif defined(__linux__)
#  define SUBPROCESS_NAME "gdcefSubProcess"
#  define NEEDED_LIBRARIES "libcef.so", "libgdcef.so", "libvulkan.so.1", \
        "libvk_swiftshader.so", "libGLESv2.so", "libEGL.so"
#elif defined(__APPLE__)
#  define SUBPROCESS_NAME "gdcefSubProcess"
#  define NEEDED_LIBRARIES "libcef.dylib", "libgdcef.dylib", "libvulkan.dylib", \
        "libvk_swiftshader.dylib", "libGLESv2.dylib", "libEGL.dylib"
#else
#  error "Undefined path for the Godot's CEF sub process for this architecture"
#endif

//------------------------------------------------------------------------------
// Logs
#define GDCEF_DEBUG()                                                      \
  std::cout << "[GDCEF][GDCEF::" << __func__ << "]" << std::endl
#define GDCEF_DEBUG_VAL(x)                                                 \
  std::cout << "[GDCEF][GDCEF::" << __func__ << "] " << x << std::endl
#define GDCEF_ERROR(x)                                                     \
  std::cerr << "[GDCEF][GDCEF::" << __func__ << "] " << x << std::endl
#define BROWSER_DEBUG()                                                    \
  std::cout << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "]"  \
            << std::endl
#define BROWSER_DEBUG_VAL(x)                                               \
  std::cout << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "] " \
            << x << std::endl
#define BROWSER_ERROR(x)                                                   \
  std::cerr << "[GDCEF][BrowserView::" << __func__ << "][" << m_id << "] " \
            << x << std::endl

//------------------------------------------------------------------------------
static void configureCEF(fs::path const& folder, CefSettings& cef_settings,
                         CefWindowInfo& window_info);
static void configureBrowser(CefBrowserSettings& browser_settings);

//------------------------------------------------------------------------------
static bool sanity_checks(fs::path const& folder)
{
    // List of needed files to make CEF working. We have to check their presence
    // and integrity (even if race condition may theim be modified or removed).
    const std::vector<std::string> files =
    {
        SUBPROCESS_NAME, NEEDED_LIBRARIES,
        "icudtl.dat", "chrome_100_percent.pak", "chrome_200_percent.pak",
        "resources.pak", "v8_context_snapshot.bin"
    };

    // Check if important CEF assets exist and are valid.
    // FIXME: perform some SHA1
    return are_valid_files(folder, files);
}

//------------------------------------------------------------------------------
static bool isPlayInEditor()
{
    return executable_name().find("godot") != std::string::npos;
}

//------------------------------------------------------------------------------
// in a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method CefRefPtr<CefBrowser> m_browser;this is used to expose various methods
// of this class to Godot
void GDCef::_register_methods()
{
    std::cout << "[GDCEF][GDCef::_register_methods]" << std::endl;

    godot::register_method("_process", &GDCef::_process);
    godot::register_method("create_browser", &GDCef::createBrowser);
}

//------------------------------------------------------------------------------
void GDCef::_init()
{
    GDCEF_DEBUG_VAL("Executable name: " << executable_name());

    // Get the folder path in which Stigmee and CEF assets are present
    fs::path folder;

    // Check if this process is executing from the Godot editor or from the
    // Stigmee standalone application.
    if (isPlayInEditor())
    {
        folder = std::filesystem::current_path() / "build";
        GDCEF_DEBUG_VAL("Launching CEF from Godot editor");
        GDCEF_DEBUG_VAL("Path where your project Godot files shall be located:"
                        << folder);
    }
    else
    {
        folder = real_path();
        GDCEF_DEBUG_VAL("Launching CEF from Stigmee executable");
        GDCEF_DEBUG_VAL("Path where your Stigmee files shall be located:"
                        << folder);
    }

    // Check if needed files to make CEF working are present.
    if (!sanity_checks(folder))
    {
        GDCEF_ERROR("Aborting because of missing necessary files");
        exit(1);
    }

    // Since we cannot configure CEF from the command line main(argc, argv)
    // because we cannot access it we configure directly.
    configureCEF(folder, m_cef_settings, m_window_info);
    configureBrowser(m_browser_settings);

    // This function should be called on the main application thread to
    // initialize the CEF browser process. The |application| parameter may be
    // empty. A return value of true indicates that it succeeded and false
    // indicates that it failed.  The |windows_sandbox_info| parameter is only
    // used on Windows and may be NULL (see cef_sandbox_win.h for details).
    CefMainArgs args;
    GDCEF_DEBUG_VAL("[GDCEF][GDCef::_init] CefInitialize");
    if (!CefInitialize(args, m_cef_settings, nullptr, nullptr))
    {
        GDCEF_ERROR("CefInitialize failed");
        exit(1);
    }
    GDCEF_DEBUG_VAL("CefInitialize done with success");
}

//------------------------------------------------------------------------------
void GDCef::_process(float /*delta*/)
{
    CefDoMessageLoopWork();
}

//------------------------------------------------------------------------------
// See workspace_stigmee/godot/gdnative/browser/thirdparty/cef_binary/include/
// internal/cef_types.h for more settings.
static void configureCEF(fs::path const& folder, CefSettings& cef_settings,
                         CefWindowInfo& window_info)
{
    // The path to a separate executable that will be launched for
    // sub-processes.  If this value is empty on Windows or Linux then the main
    // process executable will be used. If this value is empty on macOS then a
    // helper executable must exist at "Contents/Frameworks/<app>
    // Helper.app/Contents/MacOS/<app> Helper" in the top-level app bundle. See
    // the comments on CefExecuteProcess() for details. If this value is
    // non-empty then it must be an absolute path. Also configurable using the
    // "browser-subprocess-path" command-line switch.
    fs::path sub_process_path = { folder / SUBPROCESS_NAME };
    std::cout << "[GDCEF][GDCef::configureCEF] Setting SubProcess path: "
              << sub_process_path.string() << std::endl;
    CefString(&cef_settings.browser_subprocess_path)
            .FromString(sub_process_path.string());

    // The location where data for the global browser cache will be stored on
    // disk. If this value is non-empty then it must be an absolute path that is
    // either equal to or a child directory of CefSettings.root_cache_path. If
    // this value is empty then browsers will be created in "incognito mode"
    // where in-memory caches are used for storage and no data is persisted to
    // disk.  HTML5 databases such as localStorage will only persist across
    // sessions if a cache path is specified. Can be overridden for individual
    // CefRequestContext instances via the CefRequestContextSettings.cache_path
    // value. When using the Chrome runtime the "default" profile will be used
    // if |cache_path| and |root_cache_path| have the same value.
    fs::path sub_process_cache = { folder / "cache" };
    std::cout << "[GDCEF][GDCef::configureCEF] Setting cache path: "
              << sub_process_cache.string() << std::endl;
    CefString(&cef_settings.cache_path)
            .FromString(sub_process_cache.string());

    // The root directory that all CefSettings.cache_path and
    // CefRequestContextSettings.cache_path values must have in common. If this
    // value is empty and CefSettings.cache_path is non-empty then it will
    // default to the CefSettings.cache_path value. If this value is non-empty
    // then it must be an absolute path. Failure to set this value correctly may
    // result in the sandbox blocking read/write access to the cache_path
    // directory.
    CefString(&cef_settings.root_cache_path)
            .FromString(sub_process_cache.string());

    // The locale string that will be passed to WebKit. If empty the default
    // locale of "en-US" will be used. This value is ignored on Linux where
    // locale is determined using environment variable parsing with the
    // precedence order: LANGUAGE, LC_ALL, LC_MESSAGES and LANG. Also
    // configurable using the "lang" command-line switch.
    CefString(&cef_settings.locale).FromString("fr");

    // The directory and file name to use for the debug log. If empty a default
    // log file name and location will be used. On Windows and Linux a
    // "debug.log" file will be written in the main executable directory. On
    // MacOS a "~/Library/Logs/<app name>_debug.log" file will be written where
    // <app name> is the name of the main app executable. Also configurable
    // using the "log-file" command-line switch.
    CefString(&cef_settings.log_file).FromString((folder / "debug.log").string());
    cef_settings.log_severity = LOGSEVERITY_WARNING; // LOGSEVERITY_DEBUG;

    // Set to true (1) to enable windowless (off-screen) rendering support. Do
    // not enable this value if the application does not use windowless
    // rendering as it may reduce rendering performance on some systems.
    cef_settings.windowless_rendering_enabled = true;

    // Create the browser using windowless (off-screen) rendering. No window
    // will be created for the browser and all rendering will occur via the
    // CefRenderHandler interface. The |parent| value will be used to identify
    // monitor info and to act as the parent window for dialogs, context menus,
    // etc. If |parent| is not provided then the main screen monitor will be
    // used and some functionality that requires a parent window may not
    // function correctly. In order to create windowless browsers the
    // CefSettings.windowless_rendering_enabled value must be set to true.
    // Transparent painting is enabled by default but can be disabled by setting
    // CefBrowserSettings.background_color to an opaque value.
    window_info.SetAsWindowless(0);

    // To allow calling OnPaint()
    window_info.shared_texture_enabled = false;

    // Set to true (1) to disable the sandbox for sub-processes. See
    // cef_sandbox_win.h for requirements to enable the sandbox on Windows. Also
    // configurable using the "no-sandbox" command-line switch.
    cef_settings.no_sandbox = true;

    // Set to true (1) to disable configuration of browser process features
    // using standard CEF and Chromium command-line arguments. Configuration can
    // still be specified using CEF data structures or via the
    // CefApp::OnBeforeCommandLineProcessing() method.
    cef_settings.command_line_args_disabled = true;

    // Set to a value between 1024 and 65535 to enable remote debugging on the
    // specified port. For example, if 8080 is specified the remote debugging
    // URL will be http://localhost:8080. CEF can be remotely debugged from any
    // CEF or Chrome browser window. Also configurable using the
    // "remote-debugging-port" command-line switch.
    cef_settings.remote_debugging_port = 7777;

    // The number of stack trace frames to capture for uncaught exceptions.
    // Specify a positive value to enable the CefRenderProcessHandler::
    // OnUncaughtException() callback. Specify 0 (default value) and
    // OnUncaughtException() will not be called. Also configurable using the
    // "uncaught-exception-stack-size" command-line switch.
    cef_settings.uncaught_exception_stack_size = 5;

    // Set to true (1) to have the browser process message loop run in a
    // separate thread. If false (0) than the CefDoMessageLoopWork() function
    // must be called from your application message loop. This option is only
    // supported on Windows and Linux.
    cef_settings.multi_threaded_message_loop = 0;
}

//------------------------------------------------------------------------------
// See workspace_stigmee/godot/gdnative/browser/thirdparty/cef_binary/include/
// internal/cef_types.h for more settings.
static void configureBrowser(CefBrowserSettings& browser_settings)
{
    // The maximum rate in frames per second (fps) that
    // CefRenderHandler::OnPaint will be called for a windowless browser. The
    // actual fps may be lower if the browser cannot generate frames at the
    // requested rate. The minimum value is 1 and the maximum value is 60
    // (default 30). This value can also be changed dynamically via
    // CefBrowserHost::SetWindowlessFrameRate.
    browser_settings.windowless_frame_rate = 30;

    // Controls whether JavaScript can be executed. Also configurable using the
    // "disable-javascript" command-line switch.
    browser_settings.javascript = STATE_ENABLED;

    // Controls whether JavaScript can be used to close windows that were not
    // opened via JavaScript. JavaScript can still be used to close windows that
    // were opened via JavaScript or that have no back/forward history. Also
    // configurable using the "disable-javascript-close-windows" command-line
    // switch.
    browser_settings.javascript_close_windows = STATE_DISABLED;

    // Controls whether JavaScript can access the clipboard. Also configurable
    // using the "disable-javascript-access-clipboard" command-line switch.
    browser_settings.javascript_access_clipboard = STATE_DISABLED;

    // Controls whether DOM pasting is supported in the editor via
    // execCommand("paste"). The |javascript_access_clipboard| setting must also
    // be enabled. Also configurable using the "disable-javascript-dom-paste"
    // command-line switch.
    browser_settings.javascript_dom_paste = STATE_DISABLED;

    // Controls whether any plugins will be loaded. Also configurable using the
    // "disable-plugins" command-line switch.
    browser_settings.plugins = STATE_ENABLED;

    // Controls whether image URLs will be loaded from the network. A cached
    // image will still be rendered if requested. Also configurable using the
    // "disable-image-loading" command-line switch.
    browser_settings.image_loading = STATE_DISABLED;

    // Controls whether databases can be used. Also configurable using the
    // "disable-databases" command-line switch.
    browser_settings.databases = STATE_ENABLED;

    // Controls whether WebGL can be used. Note that WebGL requires hardware
    // support and may not work on all systems even when enabled. Also
    // configurable using the "disable-webgl" command-line switch.
    browser_settings.webgl = STATE_ENABLED;
}

//------------------------------------------------------------------------------
GDCef::~GDCef()
{
    GDCEF_DEBUG();
    CefShutdown();
}

//------------------------------------------------------------------------------
BrowserView* GDCef::createBrowser(godot::String const name, godot::String const url)
{
    GDCEF_DEBUG_VAL("name: " << name.utf8().get_data() <<
                    ", url: " << url.utf8().get_data());

    BrowserView* browser = BrowserView::_new();
    if (browser == nullptr)
    {
        GDCEF_ERROR("new BrowserView() failed");
        return nullptr;
    }

    // Complete BrowserView constructor
    int id = browser->init(url, settingsBrowser(), windowInfo());
    if (id < 0)
    {
       GDCEF_ERROR("browser->init() failed");
       return nullptr;
    }

    browser->set_name(name);
    add_child(browser);
    //m_browsers[id] = browser;
    return browser;
}

//------------------------------------------------------------------------------
void GDCef::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    GDCEF_DEBUG();

    // Add to the list of existing browsers.
    m_browsers[browser->GetIdentifier()] = browser;
}

//------------------------------------------------------------------------------
bool GDCef::DoClose(CefRefPtr<CefBrowser> /*browser*/)
{
    CEF_REQUIRE_UI_THREAD();
    GDCEF_DEBUG();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    if (m_browsers.size() == 1u)
    {
        // Set a flag to indicate that the window close should be allowed.
        //is_closing_ = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

//------------------------------------------------------------------------------
void GDCef::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    GDCEF_DEBUG();

    // Remove from the list of existing browsers.
    m_browsers.erase(browser->GetIdentifier());

    if (m_browsers.empty())
    {
        // All browser windows have closed. Quit the application message loop.
        //CefQuitMessageLoop();
    }
}

//------------------------------------------------------------------------------
// in a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method CefRefPtr<CefBrowser> m_browser;this is used to expose various methods of this class to Godot
void BrowserView::_register_methods()
{
    GDCEF_DEBUG();

    godot::register_method("id", &BrowserView::id);
    godot::register_method("is_valid", &BrowserView::isValid);
    godot::register_method("get_texture", &BrowserView::texture);
    godot::register_method("use_texture_from", &BrowserView::texture);
    godot::register_method("set_zoom_level", &BrowserView::setZoomLevel);
    godot::register_method("load_url", &BrowserView::loadURL);
    godot::register_method("is_loaded", &BrowserView::loaded);
    godot::register_method("get_url", &BrowserView::getURL);
    godot::register_method("stop_loading", &BrowserView::stopLoading);
    godot::register_method("can_navigate_backward", &BrowserView::canNavigateBackward);
    godot::register_method("can_navigate_forward", &BrowserView::canNavigateForward);
    godot::register_method("navigate_back", &BrowserView::navigateBackward);
    godot::register_method("navigate_forward", &BrowserView::navigateForward);
    godot::register_method("set_size", &BrowserView::reshape);
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
{
    BROWSER_DEBUG();
}

//------------------------------------------------------------------------------
int BrowserView::init(godot::String const& url, CefBrowserSettings const& settings,
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
        window_info, this, url.utf8().get_data(), settings,
        nullptr, nullptr);

    if ((m_browser == nullptr) || (m_browser->GetHost() == nullptr))
    {
        m_id = -1;
        BROWSER_ERROR("CreateBrowserSync failed");
    }
    else
    {
        m_id = m_browser->GetIdentifier();
        BROWSER_DEBUG_VAL("CreateBrowserSync succeeded");
        m_browser->GetHost()->WasResized();
    }

    return m_id;
}

//------------------------------------------------------------------------------
BrowserView::BrowserView()
    : m_viewport({ 0.0f, 0.0f, 1.0f, 1.0f})
{
    BROWSER_DEBUG_VAL("Create Godot texture");

    m_image.instance();
    m_texture.instance();
}

//------------------------------------------------------------------------------
BrowserView::~BrowserView()
{
    BROWSER_DEBUG();

    if (!m_browser)
        return ;

    auto host = m_browser->GetHost();
    if (!host)
        return ;

    host->CloseDevTools(); // remote_debugging_port
    host->TryCloseBrowser();//CloseBrowser(true);
}

//------------------------------------------------------------------------------
void BrowserView::GetViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect)
{
    BROWSER_DEBUG_VAL(int(m_viewport[0] * m_width) << ", " <<
                      int(m_viewport[1] * m_height) << ", " <<
                      int(m_viewport[2] * m_width) << ", " <<
                      int(m_viewport[3] * m_height));
    rect = CefRect(int(m_viewport[0] * m_width),
                   int(m_viewport[1] * m_height),
                   int(m_viewport[2] * m_width),
                   int(m_viewport[3] * m_height));
}

//------------------------------------------------------------------------------
// FIXME find a less naive algorithm et utiliser dirtyRects
void BrowserView::OnPaint(CefRefPtr<CefBrowser> /*browser*/, PaintElementType /*type*/,
                          const RectList& /*dirtyRects*/, const void* buffer,
                          int width, int height)
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
void BrowserView::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> /*frame*/,
                            int /*httpStatusCode*/)
{
    GDCEF_DEBUG_VAL("has ended loading");
    assert(browser != nullptr);
    assert(m_browser != nullptr);
    assert(browser->GetIdentifier() == m_browser->GetIdentifier());
    (void) browser;

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
