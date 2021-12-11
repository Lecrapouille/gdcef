//*************************************************************************
// Stigmee: A 3D browser and decentralized social network.
// Copyright 2021 Alain Duron <duron.alain@gmail.com>
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

#include "browser.h"
#include "./include/wrapper/cef_helpers.h"
#include <OS.hpp>

// Gets the button list to handle mouse events
// TODO : find the appropriate header (.hpp file in godot-cpp) to avoid this
enum ButtonList {
	BUTTON_LEFT = 1,
	BUTTON_RIGHT = 2,
	BUTTON_MIDDLE = 3,
	BUTTON_WHEEL_UP = 4,
	BUTTON_WHEEL_DOWN = 5,
	BUTTON_WHEEL_LEFT = 6,
	BUTTON_WHEEL_RIGHT = 7,
	BUTTON_XBUTTON1 = 8,
	BUTTON_XBUTTON2 = 9,
	BUTTON_MASK_LEFT = (1 << (BUTTON_LEFT - 1)),
	BUTTON_MASK_RIGHT = (1 << (BUTTON_RIGHT - 1)),
	BUTTON_MASK_MIDDLE = (1 << (BUTTON_MIDDLE - 1)),
	BUTTON_MASK_XBUTTON1 = (1 << (BUTTON_XBUTTON1 - 1)),
	BUTTON_MASK_XBUTTON2 = (1 << (BUTTON_XBUTTON2 - 1))
};


using namespace godot;

// in a GDNative module, "_bind_methods" is replaced by the "_register_methods" method
// this is used to expose various methods of this class to Godot
void BrowserView::_register_methods()
{
	register_method("load_url", &BrowserView::load_url);
	register_method("get_texture", &BrowserView::get_texture);
	register_method("reshape", &BrowserView::reshape);
	register_method("on_key_pressed", &BrowserView::keyPress);
	register_method("on_mouse_moved", &BrowserView::mouseMove);
	register_method("on_mouse_click", &BrowserView::mouseClick);
}


void BrowserView::_init() {
	// This is the entry point of GDNative for any instanciation of this class
	// Here we do nada and let the work be done by calling other exposed methods from Godot Scripts
}

//------------------------------------------------------------------------------
BrowserView::BrowserView()
	: m_mouse_x(0), m_mouse_y(0)
{
	std::cout << "BrowserView::BrowserView()" << std::endl;

	m_image.instance();
	m_texture.instance();

	CefWindowInfo window_info;

	// Testing various ways to get the HwND of the main UI
	// When using subProcess executable this is mostly useless.
	// just set windowless_rendering_enabled

	// ------------- Test 1 ---------------
	//window_info.SetAsWindowless(0);
	// ------------- Test 2 ---------------
	//OS* os = OS::get_singleton();
	//int64_t hdle = os->get_native_handle(OS::WINDOW_HANDLE);
	//window_info.SetAsWindowless((HWND)hdle);
	// ------------------------------------

	window_info.windowless_rendering_enabled = 1;
	std::cout << "[BrowserView] Creating the render handler" << std::endl;
	m_render_handler = new RenderHandler(*this);
	// initial browser's view size. we expose it to godot which can set the desired size
	// depending on its viewport size.
	m_render_handler->reshape(512, 512); 

	// Various cef settings.
	// TODO : test DPI settings
	CefBrowserSettings settings;
	settings.windowless_frame_rate = 60; // 30 is default

	std::cout << "[BrowserView] Creating the client and the browser" << std::endl;
	m_client = new BrowserClient(m_render_handler);
	m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_client.get(),
		"https://www.google.com/", settings,
		nullptr, nullptr);
	std::cout << "[BrowserView] CreateBrowserSync has been called !" << std::endl;
}

//------------------------------------------------------------------------------
BrowserView::~BrowserView()
{
	std::cout << "BrowserView::~BrowserView()" << std::endl;
	CefDoMessageLoopWork();
	m_browser->GetHost()->CloseBrowser(true);

	m_browser = nullptr;
	m_client = nullptr;
}

//------------------------------------------------------------------------------
BrowserView::RenderHandler::RenderHandler(BrowserView& owner)
	: m_owner(owner)
{}

//------------------------------------------------------------------------------
void BrowserView::RenderHandler::reshape(int w, int h)
{
	std::cout << "BrowserView::RenderHandler::reshape" << std::endl;
	m_width = w;
	m_height = h;
}

//------------------------------------------------------------------------------
void BrowserView::RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
	std::cout << "BrowserView::RenderHandler::GetViewRect" << std::endl;
	rect = CefRect(0, 0, m_width, m_height);
}

//------------------------------------------------------------------------------
void BrowserView::RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
	const RectList& dirtyRects, const void* buffer,
	int width, int height)
{
	// https://github.com/godotengine/godot/issues/42346

	std::cout << "BrowserView::RenderHandler::OnPaint" << std::endl;

	const int size = width * height * 4 * sizeof(char);

	m_data.resize(size);
	//PoolVector<uint8_t>::Write w = m_data.write();
	PoolByteArray::Write w = m_data.write();
	memcpy(&w[0], buffer, size);

	// create can be replaced by create_from_data, taking an additional argument
	// void create_from_data(const int64_t width, const int64_t height, const bool use_mipmaps, const int64_t format, const PoolByteArray data);
	m_owner.m_image->create_from_data(width, height, false, Image::FORMAT_RGBA8, m_data); // STILLNEEDTOFIXME BGRA8
	m_owner.m_texture->create_from_image(m_owner.m_image, Texture::FLAG_VIDEO_SURFACE);

}

//------------------------------------------------------------------------------
void BrowserView::load_url(const String& url)
{
	m_browser->GetMainFrame()->LoadURL(url.utf8().get_data());
}

//------------------------------------------------------------------------------
void BrowserView::reshape(int w, int h)
{
	std::cout << "BrowserView::reshape" << std::endl;
	std::cout << "m_render_handler->reshape" << std::endl;
	m_render_handler->reshape(w, h);
	std::cout << "m_browser->GetHost()->WasResized" << std::endl;
	m_browser->GetHost()->WasResized();
}

//------------------------------------------------------------------------------
void BrowserView::mouseMove(int x, int y)
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
void BrowserView::mouseClick(int button, bool mouse_up)
{
	std::cout << "[BrowserView::mouseClick] mouse event occured" << std::endl;
	CefMouseEvent evt;
	std::cout << "[BrowserView::mouseClick] x,y" << m_mouse_x << "," << m_mouse_y << std::endl;
	evt.x = m_mouse_x;
	evt.y = m_mouse_y;

	std::cout << "[BrowserView::mouseClick]" << std::endl;

	CefBrowserHost::MouseButtonType btn;
	switch (button)
	{
	case BUTTON_LEFT:
		btn = CefBrowserHost::MouseButtonType::MBT_LEFT;
		std::cout << "BrowserView::mouseClick Left " << mouse_up << std::endl;
		break;
	case BUTTON_RIGHT:
		btn = CefBrowserHost::MouseButtonType::MBT_RIGHT;
		std::cout << "BrowserView::mouseClick Right " << mouse_up << std::endl;
		break;
	case BUTTON_MIDDLE:
		btn = CefBrowserHost::MouseButtonType::MBT_MIDDLE;
		std::cout << "BrowserView::mouseClick Middle " << mouse_up << std::endl;
		break;
	default:
		return;
	}

	int click_count = 1; // TODO
	m_browser->GetHost()->SendMouseClickEvent(evt, btn, mouse_up, click_count);
}

//------------------------------------------------------------------------------
void BrowserView::keyPress(int key, bool pressed)
{

	// Not working yet, need some focus implementation
	CefKeyEvent evt;
	evt.character = key;
	evt.native_key_code = key;
	evt.type = pressed ? KEYEVENT_CHAR : KEYEVENT_KEYUP;

	m_browser->GetHost()->SendKeyEvent(evt);
}
