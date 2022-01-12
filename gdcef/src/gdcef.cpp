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

//=============================================================================
//
//=============================================================================
class Manager : public CefApp
{
public:

    virtual void OnBeforeCommandLineProcessing(
        const CefString& ProcessType,
        CefRefPtr<CefCommandLine> CommandLine) override
    {
        std::cout << "PRIMARY PRIMARY Manager::OnBeforeCommandLineProcessing" << std::endl;

        /**
         * Used to pick command line switches
         * If set to "true": CEF will use less CPU, but rendering performance will be lower. CSS3 and WebGL are not be usable
         * If set to "false": CEF will use more CPU, but rendering will be better, CSS3 and WebGL will also be usable
         */
        Manager::CPURenderSettings = false;
        Manager::AutoPlay = false;

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

        // Append more command line options here if you want
        // Visit Peter Beverloo's site: http://peter.sh/experiments/chromium-command-line-switches/ for more info on the switches
    }

    static CefSettings Settings;
    static CefMainArgs MainArgs;
    static bool CPURenderSettings;
    static bool AutoPlay;

    IMPLEMENT_REFCOUNTING(Manager);
};

CefSettings Manager::Settings;
CefMainArgs Manager::MainArgs;
bool Manager::CPURenderSettings;
bool Manager::AutoPlay;

// in a GDNative module, "_bind_methods" is replaced by the "_register_methods" method
// this is used to expose various methods of this class to Godot
void GDCef::_register_methods()
{
    std::cout << "PRIMARY GDCef::_register_methods" << std::endl;

    godot::register_method("url", &GDCef::load_url);
    godot::register_method("DoMessageLoop", &GDCef::DoMessageLoop); // FIXME should be static
    godot::register_method("get_texture", &GDCef::get_texture);
    godot::register_method("reshape", &GDCef::reshape);
    godot::register_method("on_key_pressed", &GDCef::keyPress);
    godot::register_method("on_mouse_moved", &GDCef::mouseMove);
    godot::register_method("on_mouse_click", &GDCef::mouseClick);
}

void GDCef::_init()
{
    std::cout << "PRIMARY GDCef::_init" << std::endl;

    //CefString GameDirCef = *FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "BluCache");
    //FString ExecutablePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Plugins/BLUI/ThirdParty/cef/");

    // Setup the default settings for Manager
    Manager::Settings.windowless_rendering_enabled = true;
    Manager::Settings.no_sandbox = true;
    Manager::Settings.remote_debugging_port = 7777;
    Manager::Settings.uncaught_exception_stack_size = 5;

    CefString realExePath = "/home/qq/chreage_workspace/godot-modules/build/godotcefsimple";

    // Set the sub-process path
    CefString(&Manager::Settings.browser_subprocess_path).FromString(realExePath);

    // Set the cache path
    //CefString(&Manager::Settings.cache_path).FromString(GameDirCef);

    // Make a new manager instance
    CefRefPtr<Manager> BluApp = new Manager();

    //CefExecuteProcess(Manager::main_args, BluApp, nullptr);
    CefInitialize(Manager::MainArgs, Manager::Settings, BluApp, nullptr);

    std::cout << "PRIMARY GDCef::_init done" << std::endl;


    //******* Code GDCef::GDCef() ici !!!!

    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);

    m_render_handler = new RenderHandler(*this);
    // initial browser's view size. we expose it to godot which can set the desired size
    // depending on its viewport size.
    m_render_handler->reshape(128, 128);

    // Various cef settings.
    // TODO : test DPI settings
    CefBrowserSettings settings;
    settings.windowless_frame_rate = 60; // 30 is default
    if (!Manager::CPURenderSettings)
    {
        settings.webgl = STATE_ENABLED;
    }

    std::cout << "PRIMARY [GDCef] Creating the client and the browser" << std::endl;
    m_client = new BrowserClient(m_render_handler);
    m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_client.get(),
                                                  "https://www.google.com/", settings,
                                                  nullptr, nullptr);
    std::cout << "PRIMARY [GDCef] CreateBrowserSync has been called !" << std::endl;
    std::cout << "PRIMARY GDCef::GDCef() done" << std::endl;
}

//------------------------------------------------------------------------------
GDCef::GDCef()
    : m_mouse_x(0), m_mouse_y(0)
{
    std::cout << "PRIMARY GDCef::GDCef() begin" << std::endl;
    m_image.instance();
    m_texture.instance();

#if 0
    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);

    m_render_handler = new RenderHandler(*this);
    // initial browser's view size. we expose it to godot which can set the desired size
    // depending on its viewport size.
    m_render_handler->reshape(128, 128);

    // Various cef settings.
    // TODO : test DPI settings
    CefBrowserSettings settings;
    settings.windowless_frame_rate = 60; // 30 is default
    if (!Manager::CPURenderSettings)
    {
        settings.webgl = STATE_ENABLED;
    }

    std::cout << "PRIMARY [GDCef] Creating the client and the browser" << std::endl;
    m_client = new BrowserClient(m_render_handler);
    m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_client.get(),
                                                  "https://www.google.com/", settings,
                                                  nullptr, nullptr);
    std::cout << "PRIMARY [GDCef] CreateBrowserSync has been called !" << std::endl;
    std::cout << "PRIMARY GDCef::GDCef() done" << std::endl;
#endif
}

//------------------------------------------------------------------------------
void GDCef::DoMessageLoop()
{
    std::cout << "PRIMARY GDCef::DoMessageLoop()" << std::endl;
    CefDoMessageLoopWork();
}

//------------------------------------------------------------------------------
GDCef::~GDCef()
{
    std::cout << "PRIMARY GDCef::~GDCef()" << std::endl;
    CefDoMessageLoopWork();
    m_browser->GetHost()->CloseDevTools(); // remote_debugging_port
    m_browser->GetHost()->CloseBrowser(true);

    m_browser = nullptr;
    m_client = nullptr;
}

//------------------------------------------------------------------------------
GDCef::RenderHandler::RenderHandler(GDCef& owner)
    : m_owner(owner)
{}

//------------------------------------------------------------------------------
void GDCef::RenderHandler::reshape(int w, int h)
{
    std::cout << "PRIMARY GDCef::RenderHandler::reshape" << std::endl;
    m_width = w;
    m_height = h;
}

//------------------------------------------------------------------------------
void GDCef::RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    std::cout << "PRIMARY GDCef::RenderHandler::GetViewRect" << std::endl;
    rect = CefRect(0, 0, m_width, m_height);
}

//------------------------------------------------------------------------------
// FIXME find a less naive algorithm
void GDCef::RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
                                         const RectList& dirtyRects, const void* buffer,
                                         int width, int height)
{
    std::cout << "PRIMARY GDCef::RenderHandler::OnPaint" << std::endl;
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
    std::cout << "PRIMARY GDCef::RenderHandler::load_url" << std::endl;
    m_browser->GetMainFrame()->LoadURL(url.utf8().get_data());
}

//------------------------------------------------------------------------------
void GDCef::reshape(int w, int h)
{
    std::cout << "PRIMARY GDCef::reshape" << std::endl;
    std::cout << "PRIMARY m_render_handler->reshape" << std::endl;
    m_render_handler->reshape(w, h);
    std::cout << "PRIMARY m_browser->GetHost()->WasResized" << std::endl;
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
    m_browser->GetHost()->SendMouseMoveEvent(evt, mouse_leave);
}

//------------------------------------------------------------------------------
void GDCef::mouseClick(int button, bool mouse_up)
{
    std::cout << "PRIMARY [GDCef::mouseClick] mouse event occured" << std::endl;
    CefMouseEvent evt;
    std::cout << "PRIMARY [GDCef::mouseClick] x,y" << m_mouse_x << "," << m_mouse_y << std::endl;
    evt.x = m_mouse_x;
    evt.y = m_mouse_y;

    std::cout << "PRIMARY [GDCef::mouseClick]" << std::endl;

    CefBrowserHost::MouseButtonType btn;
    switch (button)
    {
    case godot::GlobalConstants::BUTTON_LEFT:
        btn = CefBrowserHost::MouseButtonType::MBT_LEFT;
        std::cout << "PRIMARY GDCef::mouseClick Left " << mouse_up << std::endl;
        break;
    case godot::GlobalConstants::BUTTON_RIGHT:
        btn = CefBrowserHost::MouseButtonType::MBT_RIGHT;
        std::cout << "PRIMARY GDCef::mouseClick Right " << mouse_up << std::endl;
        break;
    case godot::GlobalConstants::BUTTON_MIDDLE:
        btn = CefBrowserHost::MouseButtonType::MBT_MIDDLE;
        std::cout << "PRIMARY GDCef::mouseClick Middle " << mouse_up << std::endl;
        break;
    default:
        return;
    }

    int click_count = 1; // TODO
    m_browser->GetHost()->SendMouseClickEvent(evt, btn, mouse_up, click_count);
}

//------------------------------------------------------------------------------
void GDCef::keyPress(int key, bool pressed)
{
    // Not working yet, need some focus implementation
    CefKeyEvent evt;
    evt.character = key;
    evt.native_key_code = key;
    evt.type = pressed ? KEYEVENT_CHAR : KEYEVENT_KEYUP;

    m_browser->GetHost()->SendKeyEvent(evt);
}
