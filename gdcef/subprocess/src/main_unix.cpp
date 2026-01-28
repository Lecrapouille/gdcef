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

#include "render_process.hpp"

#ifdef _WIN32
#    error "This file is only for Linux or macOS"
#endif

//------------------------------------------------------------------------------
// Entry point function for all Linux process.
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    std::cout << ::getpid() << "::" << ::getppid() << ": [SubProcess]  "
              << __FILE__ << ": " << __PRETTY_FUNCTION__ << std::endl;

    for (int i = 0; i < argc; ++i)
    {
        std::cout << "[SubProcess] arg " << i << ": " << argv[i] << std::endl;
    }

#ifdef __APPLE__
    // Load the CEF framework library at runtime instead of linking directly
    // as required by the macOS sandbox implementation.
    CefScopedLibraryLoader library_loader;
    if (!library_loader.LoadInMain())
        return 1;
#endif

    // Provide CEF with command-line arguments.
    CefMainArgs main_args(argc, argv);

    // SimpleApp implements application-level callbacks. It will create the
    // first browser instance in OnContextInitialized() after CEF has
    // initialized.
    CefRefPtr<RenderProcess> app(new RenderProcess);

    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
    // that share the same executable. This function checks the command-line
    // and, if this is a sub-process, executes the appropriate logic.
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0)
    {
        // The sub-process has completed so return here.
        return exit_code;
    }

    // Specify CEF global settings here.
    CefSettings settings;

    // Initialize CEF for the browser process.
    if (!CefInitialize(main_args, settings, app.get(), nullptr))
    {
        std::cout << "[SubProcess] Failed to initialize CEF" << std::endl;
        return CefGetExitCode();
    }

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is
    // called.
    CefRunMessageLoop();

    // Shut down CEF.
    CefShutdown();

    std::cout << "[SubProcess] is shutdown" << std::endl;
    return 0;
}