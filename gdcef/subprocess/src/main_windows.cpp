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
#include "include/base/cef_compiler_specific.h"
#include "include/base/cef_logging.h"

#ifndef _WIN32
#    error "This file is only for Windows"
#endif

//------------------------------------------------------------------------------
// Define Windows macros before including windows.h to avoid conflicts
//------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

//------------------------------------------------------------------------------
// Entry point function for all Window process.
//------------------------------------------------------------------------------
#include <windows.h>

//------------------------------------------------------------------------------
// Exception handler to capture crashes. Written to the CEF log_file shared
// with the browser process.
//------------------------------------------------------------------------------
static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    const char* name = "";
    switch (exceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        name = " (ACCESS_VIOLATION)";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        name = " (STACK_OVERFLOW)";
        break;
    case STATUS_DLL_NOT_FOUND:
        name = " (DLL_NOT_FOUND)";
        break;
    default:
        break;
    }

    LOG(ERROR) << "[SubProcess] CRASH! Exception code: 0x" << std::hex
               << exceptionInfo->ExceptionRecord->ExceptionCode << name
               << " at address: "
               << exceptionInfo->ExceptionRecord->ExceptionAddress;
    return EXCEPTION_EXECUTE_HANDLER;
}

//------------------------------------------------------------------------------
// Chromium rotates the stack canary when forking sub-processes. This frame
// stays live across CefExecuteProcess(), so it carries the same annotation as
// main() on Unix. With MSVC the macro is empty.
// https://github.com/chromiumembedded/cef/issues/3912
//------------------------------------------------------------------------------
NO_STACK_PROTECTOR
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine,
                     int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Set up exception handler to capture crashes
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);

    // Provide CEF with command-line arguments.
    CefMainArgs main_args(hInstance);

    // SimpleApp implements application-level callbacks. It will create the
    // first browser instance in OnContextInitialized() after CEF has
    // initialized.
    CefRefPtr<RenderProcess> app(new RenderProcess);

    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
    // that share the same executable. This function checks the command-line
    // and, if this is a sub-process, executes the appropriate logic.
    //
    // IMPORTANT: This executable is ONLY meant to be launched by CEF as a
    // subprocess. CefExecuteProcess will handle everything and return a valid
    // exit code (>= 0). If it returns -1, it means this exe was launched
    // incorrectly (e.g., manually by the user).
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);

    if (exit_code < 0)
    {
        // This should not happen - it means this executable was launched
        // directly instead of being spawned by CEF as a subprocess.
        // No console in a WinMain application: use the debugger output.
        OutputDebugStringA("[SubProcess] ERROR: This executable should only "
                           "be launched by CEF as a subprocess!\n");
        exit_code = 1;
    }

    return exit_code;
}