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

#ifndef GDAPPHANDLER_H
#define GDAPPHANDLER_H


#include <Godot.hpp>
#include <Node.hpp>
#include <iostream>

// Chromium Embedded Framework
#include <cef_app.h>
#include <cef_client.h>
#include <cef_app.h>
#include <cef_helpers.h>

// NOTE: This class is mostly useless and serving testing purpose

namespace godot {

class AppHandler : public CefApp, public Node
{
    GODOT_CLASS(AppHandler, Node);

public:

    void SetArgs(int argc, char* argv[])
    {
        _argc = argc;
        _argv = argv;
    }

    void OnBeforeCommandLineProcessing(const CefString& processType,
                                       CefRefPtr<CefCommandLine> commandLine)
    {
        (void)processType;

        std::cout << "[AppHandler] OnBeforeCommandLineProcessing Arguments : Setting up arguments" << std::endl;

        // Testing some static settings
        std::cout << "[AppHandler] off-screen-renderiong-enabled" << std::endl;
        commandLine->AppendSwitch("off-screen-rendering-enabled");

        std::cout << "[AppHandler] off-screen-frame-rate : 60" << std::endl;
        commandLine->AppendSwitchWithValue("off-screen-frame-rate", "60");

        std::cout << "[AppHandler] enable-anti-aliasing" << std::endl;
        commandLine->AppendSwitch("enable-font-antialiasing");

        std::cout << "[AppHandler] enable-media-stream" << std::endl;
        commandLine->AppendSwitch("enable-media-stream");

        // OPTION 1 :settings that use less CPU
        std::cout << "[AppHandler] disable-gpu" << std::endl;
        commandLine->AppendSwitch("disable-gpu");

        std::cout << "[AppHandler] disable-gpu-compositing" << std::endl;
        commandLine->AppendSwitch("disable-gpu-compositing");

        std::cout << "[AppHandler] enable-begin-frame-scheduling" << std::endl;
        commandLine->AppendSwitch("enable-begin-frame-scheduling");

        // OPTION 2 : untested, probably not working in OSR mode. Enables things like CSS3 and WebGL
        //	CommandLine->AppendSwitch("enable-gpu-rasterization");
        //	CommandLine->AppendSwitch("enable-webgl");
        //	CommandLine->AppendSwitch("disable-web-security");

        /*
        // Testing some argument parsing method
        std::string temp;

        for (int i = 0; i < _argc; i++) {
        if (i > 0) temp = temp.append(" ");
        temp = temp.append(_argv[i]);
        std::cout << temp << std::endl;
        }

        CefString allArgs(temp);
        std::cout << "[AppHandler] setting up command line" << std::endl;
        commandLine->InitFromString(allArgs);
        */
    }

    static void _register_methods();

    AppHandler();
    ~AppHandler();

    void _init(); // our initializer called by Godot

private:

    int    _argc;
    char** _argv;

    IMPLEMENT_REFCOUNTING(AppHandler);
};

}

#endif
