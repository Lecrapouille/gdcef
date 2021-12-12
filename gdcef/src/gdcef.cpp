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

#include "gdcef.h"
#include <iostream>
#include <cef_client.h>
#include <cef_app.h>
#include <cef_helpers.h>

// Returns module handle where this function is running in: EXE or DLL
// Not really needed but that might be useful in the future
HMODULE getThisModuleHandle()
{
    HMODULE hModule = NULL;
    ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        (LPCTSTR)getThisModuleHandle, &hModule);

    return hModule;
}

// in a GDNative module, "_bind_methods" is replaced by the "_register_methods" method
// this is used to expose various methods of this class to Godot
void GDCef::_register_methods()
{
    godot::register_method("do_message_loop_work", &GDCef::do_message_loop_work);
    godot::register_method("run_message_loop", &GDCef::run_message_loop);
    godot::register_method("cef_stop", &GDCef::cef_stop);
    godot::register_method("cef_start", &GDCef::cef_start);
    godot::register_method("cef_execute_process", &GDCef::cef_execute_process);
    godot::register_method("cef_initialize", &GDCef::cef_initialize);
    godot::register_method("set_app_handler", &GDCef::set_app_handler);
}

GDCef::GDCef()
{
    std::cout << "[GDCef::GDCef()]" << std::endl;
}


GDCef::~GDCef()
{
    // add your cleanup here
    cef_stop();
    m_app = nullptr; // m_app is unused as of now
}

void GDCef::_init()
{
    // This is the entry point of GDNative for any instanciation of this class
    // Here we do nothing and let the work be done by calling other exposed methods from Godot Scripts
}

void GDCef::do_message_loop_work()
{
    // Calls the Cef method to execute pending work by the CEF processes
    CefDoMessageLoopWork();
}

void GDCef::cef_stop()
{
    // Stop the CEF
    std::cerr << "[GDCef::cef_stop()]" << std::endl;
    CefShutdown();
}

void GDCef::run_message_loop()
{
    // Run the main CEF loop
    // Do not print any message here or it will flood the console
    CefRunMessageLoop();
}

void GDCef::cef_start()
{   // int argc, wchar_t** argv
    // /!\ IMPORTANT: Call this method to instantiate the CEF from godot.
    // /!\ TODO: Add the correct args as method argument, depending on the system

    // /!\ TODO: Depending on the system, initialize the args
    //CefMainArgs args(::GetModuleHandle(NULL));
    //CefMainArgs cefArgs;
    HMODULE hModule = getThisModuleHandle();
    CefMainArgs args(hModule);

    std::cerr << "[CEF_start] CefExecuteProcess(): Starting a process" << std::endl;
    int exit_code = CefExecuteProcess(args, nullptr, nullptr);
    if (exit_code >= 0)
    {
        std::cerr << "[CEF_start] CefExecuteProcess(): Chromium sub-process has completed" << std::endl;
        exit(exit_code);
    }
    else if (exit_code == -1)
    {
        std::cerr << "[CEF_start] CefExecuteProcess(): argv not for Chromium: ignoring!" << std::endl;
    }
    std::cerr << "[CEF_start] CefExecuteProcess(): done" << std::endl;

    // Configure Chromium
    CefSettings settings;
    // TODO CefString(&settings.locales_dir_path) = "cef/linux/lib/locales";
    settings.windowless_rendering_enabled = true;
    settings.multi_threaded_message_loop = false;

#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif

    // /!\ Replace the path with correct subprocess executable path
    // /!\ TODO : investigate relative path
    CefString(&settings.browser_subprocess_path).FromASCII("D:/Stigmee/godot-native/gdcef/demo/bin/win64/cefSubProcess.exe");
    bool result = CefInitialize(args, settings, nullptr, nullptr);
    if (!result)
    {
        std::cerr << "[CEF_start] CefInitialize: failed" << std::endl;
        exit(-2);
    }
    std::cerr << "[CEF_start] CefInitialize: OK" << std::endl;

}

// ---------------------------------------------------------------------------------------------------------------------
// Note : Below methods are used for testing
// ---------------------------------------------------------------------------------------------------------------------

bool GDCef::cef_initialize() //int argc, char* argv[])
{
    // /!\ IMPORTANT: This has been created to split the cef_start method into several sub parts (mostly for testing)
    // /!\ Right now this is unused as I finally found a correct way to call cef_start using subProcess Executable
    // /!\ I'm leaving this exposed for testing purpose but use cef_start instead

    std::cout << "[GDCef::cef_initialize()] gettings module handle args" << std::endl;
    CefMainArgs args(::GetModuleHandle(NULL));
    std::cout << "[GDCef::cef_initialize()] Configuring settings" << std::endl;
    CefSettings settings;
    // TODO CefString(&settings.locales_dir_path) = "cef/linux/lib/locales";
    settings.windowless_rendering_enabled = true;
    settings.multi_threaded_message_loop = false;
#if !defined(CEF_USE_SANDBOX)
    std::cout << "[GDCef::cef_initialize()] settings.no_sandbox = true" << std::endl;
    settings.no_sandbox = true;
#endif
    std::cout << "[GDCef::cef_initialize()] running CefInitialize" << std::endl;
    bool result = CefInitialize(args, settings, nullptr, nullptr);
    //bool result = CefInitialize(args, settings, m_app, nullptr);
    return result;
}

void GDCef::set_app_handler(AppHandler* hnd)
{
    // Alain: Mostly for testing purpose, optional implementation of the CefApp
    // /!\ IMPORTANT: This has been created to split the cef_start method into several sub parts (mostly for testing)
    // /!\ Right now this is unused as I finally found a correct way to call cef_start using subProcess Executable
    // /!\ I'm leaving this exposed for testing purpose, but use cef_start instead
    m_app = hnd;
}

int GDCef::cef_execute_process()
{
    // /!\ IMPORTANT: This has been created to split the cef_start method into several sub parts (mostly for testing)
    // /!\ Right now this is unused as I finally found a correct way to call cef_start using subProcess Executable
    // /!\ I'm leaving this exposed for testing purpose, but use cef_start instead
    std::cout << "[GDCef::cef_execute_process()] running with args" << std::endl;
    std::cout << "[GDCef::cef_execute_process()]   state of the handler app :" << std::endl;
    std::cout << m_app << std::endl;
    CefMainArgs args(::GetModuleHandle(NULL));
    int t = CefExecuteProcess(args, m_app, nullptr);
    return t;
}
