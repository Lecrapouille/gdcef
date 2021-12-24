// This code is a modification of the original projects that can be found at
// https://github.com/ashea-code/BluBrowser

#include "include/base/cef_logging.h"
#include "blubrowser_app.h"
#include <iostream>

#ifdef __APPLE__
#  include "include/wrapper/cef_library_loader.h"
#endif

// Entry point function for all processes.
int main(int argc, char* argv[])
{
    std::cout << ::getpid() << "::" << ::getppid() << ": SECONDARY "
              << __FILE__ << ": " << __PRETTY_FUNCTION__ << std::endl;
    for (int i = 0; i < argc; ++i)
    {
        std::cout << "SECONDARY arg " << i << ": " << argv[i] << std::endl;
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

    // SimpleApp implements application-level callbacks. It will create the first
    // browser instance in OnContextInitialized() after CEF has initialized.
    CefRefPtr<BluBrowser> app(new BluBrowser);

    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
    // that share the same executable. This function checks the command-line and,
    // if this is a sub-process, executes the appropriate logic.
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) {
        // The sub-process has completed so return here.
        return exit_code;
    }

    // Specify CEF global settings here.
    CefSettings settings;

    // Initialize CEF for the browser process.
    CefInitialize(main_args, settings, app.get(), nullptr);

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is
    // called.
    CefRunMessageLoop();

    // Shut down CEF.
    CefShutdown();

    return 0;
}
