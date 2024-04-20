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
#include "gdcef.hpp"
#include "gdbrowser.hpp"
#include "helper_files.hpp"
#include "helper_config.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

//------------------------------------------------------------------------------
// List of file libraries and artifacts mandatory to make CEF working
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
// Folder name (not the path) holding the CEF artifacts needed to make CEF working
#if (!defined(CEF_ARTIFACTS_FOLDER))
#  error "CEF_ARTIFACTS_FOLDER is not defined"
#endif

//------------------------------------------------------------------------------
static void configureCEF(fs::path const& folder, CefSettings& cef_settings,
                         CefWindowInfo& window_info, godot::Dictionary config);

//------------------------------------------------------------------------------
// Check if needed files to make CEF working are present and well formed. We
// have to check their presence and integrity (even if race condition may theim
// be modified or removed).
static bool sanity_checks(fs::path const& folder)
{
    // List of needed files.
    const std::vector<std::string> files =
    {
        SUBPROCESS_NAME, NEEDED_LIBRARIES,
        "icudtl.dat", "chrome_100_percent.pak", "chrome_200_percent.pak",
        "resources.pak", "v8_context_snapshot.bin"
    };

    // Check if important CEF artifacts exist and have correct SHA1.
    // FIXME: SHA1 not made
    return are_valid_files(folder, files);
}

//------------------------------------------------------------------------------
// In a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method CefRefPtr<CefBrowser> m_browser;this is used to expose various methods
// of this class to Godot
//void GDCef::_register_methods()
void GDCef::_bind_methods()
{
    using namespace godot;
    std::cout << "[GDCEF][GDCef::_register_methods]" << std::endl;

    ClassDB::bind_method(D_METHOD("initialize"), &GDCef::initialize);
    ClassDB::bind_method(D_METHOD("get_full_version"), &GDCef::version);
    ClassDB::bind_method(D_METHOD("get_version_part"), &GDCef::versionPart);
    ClassDB::bind_method(D_METHOD("create_browser"), &GDCef::createBrowser);
    ClassDB::bind_method(D_METHOD("shutdown"), &GDCef::shutdown);
    ClassDB::bind_method(D_METHOD("is_alive"), &GDCef::isAlive);
    ClassDB::bind_method(D_METHOD("get_error"), &GDCef::getError);
}

//------------------------------------------------------------------------------
void GDCef::_init()
{
    // Replaced by GDCef::initialize(xxx)
}

//------------------------------------------------------------------------------
bool GDCef::initialize(godot::Dictionary config)
{
    if (m_impl != nullptr)
    {
        GDCEF_ERROR("Already initialized");
        return false;
    }
    m_impl = new GDCef::Impl(*this);

    // Get the folder path in which your application and CEF artifacts are present
    fs::path folder;

    // Check if this process is executing from the Godot editor or from the
    // your standalone application.
    if (isStartedFromGodotEditor())
    {
        std::string cef_folder_path =
                getConfig(config, "artifacts", std::string(CEF_ARTIFACTS_FOLDER));
        if (cef_folder_path.rfind("res://", 0) == 0)
        {
            // Note: exported projects don't support globalize_path, see:
            // https://docs.godotengine.org/en/3.5/classes/class_projectsettings.html
            // Section: class-projectsettings-method-globalize-path
            folder = globalize_path(cef_folder_path);
        }
        else
        {
            folder = std::filesystem::current_path() / cef_folder_path;
        }
        GDCEF_DEBUG_VAL("Launching CEF from Godot editor");
        GDCEF_DEBUG_VAL("Path where your project Godot files shall be located:"
                        << folder);
    }
    else
    {
        folder = getConfig(config, "exported_artifacts", real_path());
        GDCEF_DEBUG_VAL("Launching CEF from your executable");
        GDCEF_DEBUG_VAL("Path where your application files shall be located:"
                        << folder);
    }

    // Check if needed files to make CEF working are present.
    if (!sanity_checks(folder))
    {
        GDCEF_ERROR("Error: at least one CEF artifacts not found at path: " << folder);
        m_impl = nullptr;
        return false;
    }

    // Since we cannot configure CEF from the command line main(argc, argv)
    // because we cannot access to it, we have to configure CEF directly.
    configureCEF(folder, m_cef_settings, m_window_info, config);
    m_enable_media_stream = getConfig(config, "enable_media_stream", false);

    // This function should be called on the main application thread to
    // initialize the CEF browser process. A return value of true indicates
    // that it succeeded and false indicates that it failed.
    // Note: passed m_impl as 3th argument (as CefApp) because this is needed
    // to call OnBeforeCommandLineProcessing().
    CefMainArgs args;
    GDCEF_DEBUG_VAL("[GDCEF][GDCef::_init] CefInitialize");
    if (!CefInitialize(args, m_cef_settings, m_impl, nullptr))
    {
        GDCEF_ERROR("CefInitialize failed");
        m_impl = nullptr;
        return false;
    }
    GDCEF_DEBUG_VAL("CefInitialize done with success");
    return true;
}

//------------------------------------------------------------------------------
bool GDCef::isAlive()
{
    return m_impl != nullptr;
}

//------------------------------------------------------------------------------
int GDCef::versionPart(int entry)
{
    return cef_version_info(entry);
}

//------------------------------------------------------------------------------
godot::String GDCef::version()
{
    return CEF_VERSION;
}

//------------------------------------------------------------------------------
godot::String GDCef::getError()
{
    std::string err = m_error.str();
    m_error.clear();
    return {err.c_str()};
}

//------------------------------------------------------------------------------
void GDCef::_process(double /*delta*/)
{
    if (m_impl != nullptr)
    {
        CefDoMessageLoopWork();
    }
}

//------------------------------------------------------------------------------
//! \brief See gdcef/addons/gdcef/thirdparty/cef_binary/include/internal/cef_types.h
//! for more information about settings.
static void configureCEF(fs::path const& folder, CefSettings& cef_settings,
                         CefWindowInfo& window_info, godot::Dictionary config)
{
    // The path to a separate executable that will be launched for
    // sub-processes.  If this value is empty on Windows or Linux then the main
    // process executable will be used. If this value is empty on macOS then a
    // helper executable must exist at "Contents/Frameworks/<app>
    // Helper.app/Contents/MacOS/<app> Helper" in the top-level app bundle. See
    // the comments on CefExecuteProcess() for details. If this value is
    // non-empty then it must be an absolute path. Also configurable using the
    // "browser-subprocess-path" command-line switch.
    fs::path sub_process_path =
        getConfig(config, "browser_subprocess_path", folder / SUBPROCESS_NAME);

    std::cout << "[GDCEF][GDCef::configureCEF] Setting SubProcess path: "
              << sub_process_path.string() << std::endl;
    CefString(&cef_settings.browser_subprocess_path)
            .FromString(sub_process_path.string());

    // Incognito mode: cache directories not used.
    if (!getConfig(config, "incognito", false))
    {
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
        fs::path sub_process_cache =
            getConfig(config, "cache_path", folder / "cache");

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
        fs::path root_cache = getConfig(config, "root_cache_path", sub_process_cache);
        CefString(&cef_settings.root_cache_path)
                .FromString(root_cache.string());
    }

    // The locale string that will be passed to WebKit. If empty the default
    // locale of "en-US" will be used. This value is ignored on Linux where
    // locale is determined using environment variable parsing with the
    // precedence order: LANGUAGE, LC_ALL, LC_MESSAGES and LANG. Also
    // configurable using the "lang" command-line switch.
    std::string locale = getConfig(config, "locale", std::string("en-US"));
    CefString(&cef_settings.locale).FromString(locale);

    // The directory and file name to use for the debug log. If empty a default
    // log file name and location will be used. On Windows and Linux a
    // "debug.log" file will be written in the main executable directory. On
    // MacOS a "~/Library/Logs/<app name>_debug.log" file will be written where
    // <app name> is the name of the main app executable. Also configurable
    // using the "log-file" command-line switch.
    fs::path log_file_path = getConfig(config, "log_file", folder / "debug.log");
    CefString(&cef_settings.log_file).FromString(log_file_path.string());
    std::string logString = getConfig(config, "log_severity", std::string("warning"));
    if (logString == "verbose")
        cef_settings.log_severity = LOGSEVERITY_VERBOSE;
    else if (logString == "info")
        cef_settings.log_severity = LOGSEVERITY_INFO;
    else if (logString == "warning")
        cef_settings.log_severity = LOGSEVERITY_WARNING;
    else if (logString == "error")
        cef_settings.log_severity = LOGSEVERITY_ERROR;
    else if (logString == "fatal")
        cef_settings.log_severity = LOGSEVERITY_FATAL;

    // Set to true (1) to enable windowless (off-screen) rendering support. Do
    // not enable this value if the application does not use windowless
    // rendering as it may reduce rendering performance on some systems.
    cef_settings.windowless_rendering_enabled = true;
        //getConfig(config, "windowless_rendering_enabled", true);

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
        // getConfig(config, "shared_texture_enabled", false);

    // Set to true (1) to disable the sandbox for sub-processes. See
    // cef_sandbox_win.h for requirements to enable the sandbox on Windows. Also
    // configurable using the "no-sandbox" command-line switch.
    cef_settings.no_sandbox = true;
        // getConfig(config, "no_sandbox", true);

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
    cef_settings.remote_debugging_port =
        getConfig(config, "remote_debugging_port", 7777);

    // The number of stack trace frames to capture for uncaught exceptions.
    // Specify a positive value to enable the CefRenderProcessHandler::
    // OnUncaughtException() callback. Specify 0 (default value) and
    // OnUncaughtException() will not be called. Also configurable using the
    // "uncaught-exception-stack-size" command-line switch.
    cef_settings.uncaught_exception_stack_size =
        getConfig(config, "exception_stack_size", 5);

    // Set to true (1) to have the browser process message loop run in a
    // separate thread. If false (0) than the CefDoMessageLoopWork() function
    // must be called from your application message loop. This option is only
    // supported on Windows and Linux.
    cef_settings.multi_threaded_message_loop = 0;
        // getConfig(config, "multi_threaded_message_loop", 0);
}

//------------------------------------------------------------------------------
//! \brief See gdcef/addons/gdcef/thirdparty/cef_binary/include/ internal/cef_types.h
//! for more information about settings.
static void configureBrowser(CefBrowserSettings& browser_settings,
                             godot::Dictionary config)
{
    // The maximum rate in frames per second (fps) that
    // CefRenderHandler::OnPaint will be called for a windowless browser. The
    // actual fps may be lower if the browser cannot generate frames at the
    // requested rate. The minimum value is 1 and the maximum value is 60
    // (default 30). This value can also be changed dynamically via
    // CefBrowserHost::SetWindowlessFrameRate.
    browser_settings.windowless_frame_rate =
        getConfig(config, "frame_rate", 30);

    // Controls whether JavaScript can be executed. Also configurable using the
    // "disable-javascript" command-line switch.
    browser_settings.javascript =
        getConfig(config, "javascript", STATE_ENABLED);

    // Controls whether JavaScript can be used to close windows that were not
    // opened via JavaScript. JavaScript can still be used to close windows that
    // were opened via JavaScript or that have no back/forward history. Also
    // configurable using the "disable-javascript-close-windows" command-line
    // switch.
    browser_settings.javascript_close_windows =
        getConfig(config, "javascript_close_windows", STATE_DISABLED);

    // Controls whether JavaScript can access the clipboard. Also configurable
    // using the "disable-javascript-access-clipboard" command-line switch.
    browser_settings.javascript_access_clipboard =
        getConfig(config, "javascript_access_clipboard", STATE_DISABLED);

    // Controls whether DOM pasting is supported in the editor via
    // execCommand("paste"). The |javascript_access_clipboard| setting must also
    // be enabled. Also configurable using the "disable-javascript-dom-paste"
    // command-line switch.
    browser_settings.javascript_dom_paste =
        getConfig(config, "javascript_dom_paste", STATE_DISABLED);

    // Controls whether any plugins will be loaded. Also configurable using the
    // "disable-plugins" command-line switch.
    //  browser_settings.plugins = getConfig(config, "plugins", STATE_ENABLED);


    // Controls whether image URLs will be loaded from the network. A cached
    // image will still be rendered if requested. Also configurable using the
    // "disable-image-loading" command-line switch.
    browser_settings.image_loading =
        getConfig(config, "image_loading", STATE_ENABLED);

    // Controls whether databases can be used. Also configurable using the
    // "disable-databases" command-line switch.
    browser_settings.databases = getConfig(config, "databases", STATE_ENABLED);

    // Controls whether WebGL can be used. Note that WebGL requires hardware
    // support and may not work on all systems even when enabled. Also
    // configurable using the "disable-webgl" command-line switch.
    browser_settings.webgl = getConfig(config, "webgl", STATE_ENABLED);
}

//------------------------------------------------------------------------------
GDCef::~GDCef()
{
    shutdown();
}

//------------------------------------------------------------------------------
void GDCef::shutdown()
{
    GDCEF_DEBUG();
    if (m_impl != nullptr)
    {
        CefQuitMessageLoop();
    }
    m_impl = nullptr;
}

//------------------------------------------------------------------------------
GDBrowserView* GDCef::createBrowser(godot::String const& url,
                                    godot::TextureRect* texture_rect,
                                    godot::Dictionary config)
{
    if (m_impl == nullptr)
    {
        GDCEF_ERROR("CEF was not created (memory allocation issue)");
        return nullptr;
    }
    if (texture_rect == nullptr)
    {
        GDCEF_ERROR("You have passed a nullptr texture rectangle");
        return nullptr;
    }

    // Godot node creation (note Godot cannot pass arguments to _new())
    GDBrowserView* browser = memnew(GDBrowserView());

    // Complete BrowserView constructor (complete _new())
    CefBrowserSettings settings;
    configureBrowser(settings, config);
    int id = browser->init(url, settings, windowInfo());
    if (id < 0)
    {
        GDCEF_ERROR("browser->init() failed");
        return nullptr;
    }

    // Update the dimension of the page to the texture size
    browser->resize(texture_rect->get_size());
    texture_rect->set_texture(browser->m_texture);

    // Attach the new Godot node as child node (sadly Godot does not show created
    // nodes at run time)
    add_child(browser);

    godot::String name = browser->get_name();
    GDCEF_DEBUG_VAL("name: " << name.utf8().get_data() << ", url: " << url.utf8().get_data());
    return browser;
}

//------------------------------------------------------------------------------
void GDCef::Impl::OnAfterCreated(CefRefPtr<CefBrowser> /*browser*/)
{
    CEF_REQUIRE_UI_THREAD();
    GDCEF_DEBUG();

    // Add to the list of existing browsers.
    //m_browsers[browser->GetIdentifier()] = browser;
}

//------------------------------------------------------------------------------
bool GDCef::Impl::DoClose(CefRefPtr<CefBrowser> /*browser*/)
{
    CEF_REQUIRE_UI_THREAD();
    GDCEF_DEBUG();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    //if (m_browsers.size() == 1u)
    {
        // Set a flag to indicate that the window close should be allowed.
        //is_closing_ = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

//------------------------------------------------------------------------------
void GDCef::Impl::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    GDCEF_DEBUG();

    // Remove from the list of existing browsers from the Godot child.
    // FIXME we suppose that all child node are BrowserView.
    int64_t i = m_owner.get_child_count();
    while (i--)
    {
        godot::Node* node = m_owner.get_child(i);
        GDBrowserView* b = reinterpret_cast<GDBrowserView*>(node);
        if ((b != nullptr) && (b->id() == browser->GetIdentifier()))
        {
            GDCEF_DEBUG_VAL("Removed browser ID " << b->id());
            m_owner.remove_child(node);
            node->queue_free();
        }
    }
}

//------------------------------------------------------------------------------
void GDCef::Impl::OnBeforeCommandLineProcessing(const CefString& ProcessType,
               CefRefPtr<CefCommandLine> command_line)
{
    CEF_REQUIRE_UI_THREAD();
    GDCEF_DEBUG();

    if (command_line == nullptr)
        return ;

    // Allow accessing to the camera and microphones.
    // See https://github.com/Lecrapouille/gdcef/issues/49
    if (m_owner.m_enable_media_stream)
    {
        GDCEF_DEBUG_VAL("Allow enable-media-stream");
        command_line->AppendSwitch("enable-media-stream");
    }
}
