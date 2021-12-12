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

#ifndef GDCEF_H
#define GDCEF_H

#include <Godot.hpp>
#include <GDScript.hpp>
#include <iostream>

// Chromium Embedded Framework
//#include <cef_render_handler.h>
//#include <cef_client.h>
#include <cef_app.h>
#include <cef_client.h>
#include <cef_app.h>
#include <cef_helpers.h>
//QQ #include "apphandler.h"

// This class must be instantiated prior to the browser view node
// In theory, this should be started after the main UI thread of the game, to avoid duplicating the window for each CEF subprocess.
// I have tested several instanciation methods from godot that would avoid modifying the main.cpp of godot.
//     - autoloads (suggestion from gdnative connoisseurs in godot's discord) are of no use, they extend Node and therefore are started from the main UI
// 	   - static variable in the DLL to instantiate CEF is dangerous (leakage) and non functional, see:
//         https://magpcss.org/ceforum/viewtopic.php?f=6&t=12065&sid=ac9bb443238c0cd5f5b994131734a0fa&start=10#p22628
//     - changing the project's MainLoop (project settings) is also useless, as it's also started after the autoloads, also from the main UI
// 	   - I tried implementing CefApp just like BLUI. But it seems they finaly resolved the issue in Unreal with the same subprocess exe idea, see:
//         https://github.com/oivio/BLUI/blob/master/Source/Blu/Private/Blu.cpp
// 	   - I tried extending other classes in the hope that it would start before the main UI (GDScript, Reference...) to no avail.
//     - The ONLY workaround is to use CEF Subprocesses, see:
//         https://youtu.be/q4TRdCHe_oc

class GDCef : public godot::GDScript
{
    // I chose to extend GDScript for testing, but could as well use Node (Node.hpp)
private:

    GODOT_CLASS(GDCef, godot::GDScript); // mapping to parent class during registration.

private:

    //QQ AppHandler* m_app; // /!\ TESTING : Used along with AppHandler class below.

public:

    static void _register_methods(); // This is mandatory to expose the below methods to GDScript
    void _init(); // our initializer called by Godot. Empty, use cef_start afterwards

    GDCef();
    ~GDCef(); // Kill!Kill!DIEEE!MUERTE

    void cef_start(); // exposed, should be called to start cef
    void run_message_loop(); // create the message pump after cef initialization
    void do_message_loop_work(); // loop runner (should be using the UIT)
    void cef_stop(); // exposed, should be called to stop cef


    // --- Testing purpose only, splits up cef_start -----
    //QQ void set_app_handler(AppHandler* hnd);	// /!\ TESTING
    int cef_execute_process();				// /!\ TESTING
    bool cef_initialize();					// /!\ TESTING


    /* NOTE : Optional CefApp implementation, used for testing
       private :

       class AppHandler : public CefApp {
       public:

       void SetArgs(int argc, char* argv[]) {
       _argc = argc;
       _argv = argv;
       }

       void OnBeforeCommandLineProcessing(const CefString& processType, CefRefPtr<CefCommandLine> commandLine) {
       (void)processType;

       std::cout << "[AppHandler] OnBeforeCommandLineProcessing Arguments :" << std::endl;
       std::string temp;
       for (int i = 0; i < _argc; i++) {
       if (i > 0) temp = temp.append(" ");
       temp = temp.append(_argv[i]);
       std::cout << temp << std::endl;
       }

       CefString allArgs(temp);
       std::cout << "[AppHandler] setting up command line" << std::endl;
       commandLine->InitFromString(allArgs);
       }

       private:
       int    _argc;
       char** _argv;

       IMPLEMENT_REFCOUNTING(AppHandler);
       };

       public :

       void create_app_handler();

    */
};

#endif
