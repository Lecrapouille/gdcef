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

#include "gdcef.h"

// Godot
#include "GlobalConstants.hpp"

// CEF
#include "wrapper/cef_helpers.h"

#include <iostream>
#include <filesystem>

//------------------------------------------------------------------------------
#if defined(_WIN32)
#  define SUBPROCESS_NAME "gdcefSubProcess.exe"
#elif defined(__linux__)
#  define SUBPROCESS_NAME "gdcefSubProcess"
#elif defined(__APPLE__)
#  define SUBPROCESS_NAME "gdcefSubProcess"
#else
#  error "Undefined path for the Godot's CEF sub process for this architecture"
#endif

//------------------------------------------------------------------------------
// Init Manager static members
CefSettings GDCef::Manager::Settings;
CefMainArgs GDCef::Manager::MainArgs;
bool GDCef::Manager::CPURenderSettings;
bool GDCef::Manager::AutoPlay;

//------------------------------------------------------------------------------
void GDCef::Manager::OnBeforeCommandLineProcessing(
    const CefString& ProcessType, CefRefPtr<CefCommandLine> CommandLine)
{
    std::cout << "[GDCEF] [GDCef::Manager::OnBeforeCommandLineProcessing]" << std::endl;

    /**
     * Used to pick command line switches:
     * - If set to "true": CEF will use less CPU, but rendering performance will
     *   be lower. CSS3 and WebGL will not be usable.
     * - If set to "false": CEF will use more CPU, but rendering will be better,
     *   CSS3 and WebGL will also be usable.
     */
    GDCef::Manager::CPURenderSettings = false;
    GDCef::Manager::AutoPlay = false;

    CommandLine->AppendSwitch("off-screen-rendering-enabled");
    CommandLine->AppendSwitchWithValue("off-screen-frame-rate", "60");
    CommandLine->AppendSwitch("enable-font-antialiasing");
    CommandLine->AppendSwitch("enable-media-stream");

    // Should we use the render settings that use less CPU?
    if (CPURenderSettings)
    {
        CommandLine->AppendSwitch("disable-gpu");
        CommandLine->AppendSwitch("disable-gpu-compositing");
        CommandLine->AppendSwitch("enable-begin-frame-scheduling");
    }
    else
    {
        // Enables things like CSS3 and WebGL
        CommandLine->AppendSwitch("enable-gpu-rasterization");
        CommandLine->AppendSwitch("enable-webgl");
        CommandLine->AppendSwitch("disable-web-security");
    }

    CommandLine->AppendSwitchWithValue("enable-blink-features", "HTMLImports");

    if (AutoPlay)
    {
        CommandLine->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
    }

    // Append more command line options here if you want Visit Peter Beverloo's
    // site: http://peter.sh/experiments/chromium-command-line-switches/ for
    // more info on the switches
}

//------------------------------------------------------------------------------
// in a GDNative module, "_bind_methods" is replaced by the "_register_methods"
// method this is used to expose various methods of this class to Godot
void GDCef::_register_methods()
{
    std::cout << "[GDCEF] [GDCef::_register_methods]" << std::endl;

    godot::register_method("load_url", &GDCef::load_url); // Compat with existing name
    godot::register_method("navigate_back", &GDCef::navigate_back);
    godot::register_method("navigate_forward", &GDCef::navigate_forward);
    godot::register_method("do_message_loop_work", &GDCef::DoMessageLoopWork); // FIXME should be static, compat with existing name
    godot::register_method("get_texture", &GDCef::get_texture);
    godot::register_method("reshape", &GDCef::reshape);
    godot::register_method("on_key_pressed", &GDCef::keyPress);
    godot::register_method("on_mouse_moved", &GDCef::mouseMove);
    godot::register_method("on_mouse_click", &GDCef::mouseClick);
    godot::register_method("on_mouse_wheel", &GDCef::mouseWheel);
}

//------------------------------------------------------------------------------
static bool sanity_files(std::filesystem::path const& path)
{
    const std::vector<std::string> files = {
        SUBPROCESS_NAME, "icudtl.dat",
        "chrome_100_percent.pak", "chrome_200_percent.pak",
    };

    bool failure = false;
    for (auto const& it: files)
    {
        std::filesystem::path f = { path / it };
        if (!std::filesystem::exists(f))
        {
            std::cout << "[GDCEF] [sanity_files] File "
                      << f << " does not exist"
                      << std::endl;
            failure = true;
        }
    }

    return failure;
}

//------------------------------------------------------------------------------
void GDCef::_init()
{
    std::cout << "[GDCEF] [GDCef::_init] start" << std::endl;

    // Create the path of the CEF sub process which shall be next to Stigmee
    // binary.
    // TODO Call std::filesystem::cannonical(argv[0]) to get the real path and
    // remove symlinks.
    std::filesystem::path sub_process_path = {
        std::filesystem::current_path() / SUBPROCESS_NAME
    };
    std::cout << "[GDCEF] [GDCef::_init] Looking for SubProcess at : "
              << sub_process_path.string() << std::endl;

    // Check if important CEF assets exist
    if (sanity_files(std::filesystem::current_path()))
    {
        std::cout << "Aborting because of missing necessary files"
                  << std::endl;
        exit(1);
    }

    // Setup the default settings for GDCef::Manager
    GDCef::Manager::Settings.windowless_rendering_enabled = true;
    GDCef::Manager::Settings.no_sandbox = true;
    GDCef::Manager::Settings.remote_debugging_port = 7777;
    GDCef::Manager::Settings.uncaught_exception_stack_size = 5;

    // Set the sub-process path
    CefString(&GDCef::Manager::Settings.browser_subprocess_path)
            .FromString(sub_process_path.string());

    // TODO : Set the cache path
    //CefString(&GDCef::Manager::Settings.cache_path).FromString(GameDirCef);

    // Make a new GDCef::Manager instance
    std::cout << "[GDCEF] [GDCef::_init] Creating the CefApp (GDCef::Manager) instance"
              << std::endl;
    CefRefPtr<GDCef::Manager> GDCefApp = new GDCef::Manager();
    CefInitialize(GDCef::Manager::MainArgs, GDCef::Manager::Settings, GDCefApp, nullptr);
    std::cout << "[GDCEF] [GDCef::_init] CefInitialize done" << std::endl;

    // FIXME: ideally this code should go to GDCef::GDCef() but that does not work
    // Browser Setup
    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);
    std::cout << "[GDCEF] [GDCef::_init] Creating render handler" << std::endl;
    m_render_handler = new RenderHandler(*this);
    // initial browser's view size. we expose it to godot which can set the
    // desired size depending on its viewport size.
    m_render_handler->reshape(128, 128);

    // Various cef settings.
    // TODO : test DPI settings
    CefBrowserSettings settings;
    settings.windowless_frame_rate = 60; // 30 is default
    if (!GDCef::Manager::CPURenderSettings)
    {
        settings.webgl = STATE_ENABLED;
    }

    // TODO manage the browser class separately to avoid loading google every time
    std::cout << "[GDCEF] [GDCef::_init] Creating the client" << std::endl;
    m_client = new BrowserClient(m_render_handler);
    m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_client.get(),
                                                 "https://www.google.com/", settings,
                                                  nullptr, nullptr);
    std::cout << "[GDCEF] [GDCef::_init] CreateBrowserSync has been called !" << std::endl;
    std::cout << "[GDCEF] [GDCef::_init] done" << std::endl;
    // FIXME: here should be the end of GDCef::GDCef()
}

//------------------------------------------------------------------------------
GDCef::GDCef()
    : m_mouse_x(0), m_mouse_y(0)
{
    std::cout << "[GDCEF] [GDCef::GDCef] begin" << std::endl;
    m_image.instance();
    m_texture.instance();
}

//------------------------------------------------------------------------------
void GDCef::DoMessageLoopWork()
{
    CefDoMessageLoopWork();
}

//------------------------------------------------------------------------------
GDCef::~GDCef()
{
    std::cout << "[GDCEF] [GDCef::~GDCef()]" << std::endl;
    CefDoMessageLoopWork();
    m_browser->GetHost()->CloseDevTools(); // remote_debugging_port
    m_browser->GetHost()->CloseBrowser(true);

    m_browser = nullptr;
    m_client = nullptr;

    // Clean Shutdown of CEF
    CefShutdown();
}

//------------------------------------------------------------------------------
GDCef::RenderHandler::RenderHandler(GDCef& owner)
    : m_owner(owner)
{}

//------------------------------------------------------------------------------
void GDCef::RenderHandler::reshape(int w, int h)
{
    std::cout << "[GDCEF] [GDCef::RenderHandler::reshape]" << std::endl;
    m_width = w;
    m_height = h;
}

//------------------------------------------------------------------------------
void GDCef::RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    rect = CefRect(0, 0, m_width, m_height);
}

//------------------------------------------------------------------------------
// FIXME find a less naive algorithm et utiliser dirtyRects
void GDCef::RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
                                   const RectList& dirtyRects, const void* buffer,
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
    memcpy(&w[0], buffer, TEXTURE_SIZE);

    // Color conversion BGRA8 -> RGBA8: swap B and R chanels
    for (int i = 0; i < TEXTURE_SIZE; i += COLOR_CHANELS)
    {
        std::swap(w[i], w[i + 2]);
    }

    // Copy Godot PoolVector to Godot texture.
    m_owner.m_image->create_from_data(width, height, false, godot::Image::FORMAT_RGBA8, m_data);
    m_owner.m_texture->create_from_image(m_owner.m_image, godot::Texture::FLAG_VIDEO_SURFACE);
}

//------------------------------------------------------------------------------
void GDCef::load_url(godot::String url)
{
    std::cout << "[GDCEF] [GDCef::load_url] " << url.utf8().get_data() << std::endl;
    m_browser->GetMainFrame()->LoadURL(url.utf8().get_data());
}

//------------------------------------------------------------------------------
void GDCef::navigate_back()
{
    std::cout << "[GDCEF] [GDCef::navigate_back]" << std::endl;
    if (m_browser->CanGoBack())
    {
        m_browser->GoBack();
    }
}

//------------------------------------------------------------------------------
void GDCef::navigate_forward()
{
    std::cout << "[GDCEF] [GDCef::navigate_forward]" << std::endl;
    if (m_browser->CanGoForward())
    {
        m_browser->GoForward();
    }
}

//------------------------------------------------------------------------------
void GDCef::reshape(int w, int h)
{
    std::cout << "[GDCEF] [GDCef::reshape]" << std::endl;
    std::cout << "[GDCEF] [GDCef::reshape] m_render_handler->reshape" << std::endl;
    m_render_handler->reshape(w, h);
    std::cout << "[GDCEF] [GDCef::reshape] m_browser->GetHost()->WasResized" << std::endl;
    m_browser->GetHost()->WasResized();
}

//------------------------------------------------------------------------------
void GDCef::mouseMove(int x, int y)
{
    m_mouse_x = x;
    m_mouse_y = y;

    CefMouseEvent evt;
    evt.x = x;
    evt.y = y;

    bool mouse_leave = false; // TODO
    // AD - Adding focus just like what's done in BLUI
    m_browser->GetHost()->SetFocus(true);
    m_browser->GetHost()->SendMouseMoveEvent(evt, mouse_leave);
}

//------------------------------------------------------------------------------
void GDCef::mouseClick(int button, bool mouse_up)
{
    std::cout << "[GDCEF] [GDCef::mouseClick] mouse event occured" << std::endl;
    CefMouseEvent evt;
    std::cout << "[GDCEF] [GDCef::mouseClick] x,y" << m_mouse_x << "," << m_mouse_y << std::endl;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;

    std::cout << "[GDCEF] [GDCef::mouseClick]" << std::endl;

    CefBrowserHost::MouseButtonType btn;
    switch (button)
    {
    case godot::GlobalConstants::BUTTON_LEFT:
        btn = CefBrowserHost::MouseButtonType::MBT_LEFT;
        std::cout << "[GDCEF] [GDCef::mouseClick] Left " << mouse_up << std::endl;
        break;
    case godot::GlobalConstants::BUTTON_RIGHT:
        btn = CefBrowserHost::MouseButtonType::MBT_RIGHT;
        std::cout << "[GDCEF] [GDCef::mouseClick] Right " << mouse_up << std::endl;
        break;
    case godot::GlobalConstants::BUTTON_MIDDLE:
        btn = CefBrowserHost::MouseButtonType::MBT_MIDDLE;
        std::cout << "[GDCEF] [GDCef::mouseClick] Middle " << mouse_up << std::endl;
        break;
    default:
        return;
    }

    int click_count = 1; // TODO
    m_browser->GetHost()->SendMouseClickEvent(evt, btn, mouse_up, click_count);
}

//------------------------------------------------------------------------------
void GDCef::mouseWheel(const int wDelta)
{
    std::cout << "[GDCEF] [GDCef::mouseWheel] mouse wheel rolled" << std::endl;
    CefMouseEvent evt;
    std::cout << "[GDCEF] [GDCef::mouseWheel] x,y,wDelta : [" << m_mouse_x << ","
              << m_mouse_y << "," << wDelta << "]" << std::endl;

    evt.x = m_mouse_x;
    evt.y = m_mouse_y;

    m_browser->GetHost()->SendMouseWheelEvent(evt, wDelta * 10, wDelta * 10);
}

//------------------------------------------------------------------------------
void GDCef::keyPress(int key, bool pressed, bool isup)
{
    // Not working yet, need some focus implementation
    CefKeyEvent evtdown;
    CefKeyEvent evtup;

    evtdown.character = key;
    evtdown.windows_key_code = key;
    evtdown.native_key_code = key;
    evtdown.type = pressed ? KEYEVENT_CHAR : KEYEVENT_KEYDOWN;

    evtup.character = key;
    evtup.windows_key_code = key;
    evtup.native_key_code = key;
    evtup.type = KEYEVENT_KEYUP;

    //m_browser->GetHost()->SetFocus(true);
    if (pressed)
    {
        m_browser->GetHost()->SendKeyEvent(evtdown);
        m_browser->GetHost()->SendKeyEvent(evtup);
    }
    else
    {
        if (isup)
        {
            m_browser->GetHost()->SendKeyEvent(evtup);
        }
        else
        {
            m_browser->GetHost()->SendKeyEvent(evtdown);
        }
    }
}
