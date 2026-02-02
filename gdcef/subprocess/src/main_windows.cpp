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
#include <process.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <windows.h>
#include <fstream>
#include <sstream>

//------------------------------------------------------------------------------
// Debug logging for Windows subprocess (writes to file since no console)
//------------------------------------------------------------------------------
static std::ofstream g_log_file;

static void InitLogFile()
{
    // Get the executable directory
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exe_path(path);
    std::string log_path = exe_path.substr(0, exe_path.find_last_of("\\/")) + "\\subprocess_debug.log";
    g_log_file.open(log_path, std::ios::app);
    if (g_log_file.is_open())
    {
        g_log_file << "=== SubProcess started ===" << std::endl;
        g_log_file.flush();
    }
}

static void LogDebug(const std::string& msg)
{
    if (g_log_file.is_open())
    {
        g_log_file << "[SubProcess] " << msg << std::endl;
        g_log_file.flush();
    }
    // Also output to Visual Studio debugger
    OutputDebugStringA(("[SubProcess] " + msg + "\n").c_str());
}

//------------------------------------------------------------------------------
// Exception handler to capture crashes
//------------------------------------------------------------------------------
static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    std::stringstream ss;
    ss << "CRASH! Exception code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode
       << " at address: 0x" << exceptionInfo->ExceptionRecord->ExceptionAddress;
    LogDebug(ss.str());

    // Log common exception codes
    switch (exceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        LogDebug("Exception: ACCESS_VIOLATION");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        LogDebug("Exception: STACK_OVERFLOW");
        break;
    case STATUS_DLL_NOT_FOUND:
        LogDebug("Exception: DLL_NOT_FOUND");
        break;
    default:
        break;
    }

    if (g_log_file.is_open())
    {
        g_log_file.close();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

//------------------------------------------------------------------------------
DWORD getppid()
{
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    DWORD ppid = 0, pid = GetCurrentProcessId();

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    __try
    {
        if (hSnapshot == INVALID_HANDLE_VALUE)
            __leave;

        ZeroMemory(&pe32, sizeof(pe32));
        pe32.dwSize = sizeof(pe32);
        if (!Process32First(hSnapshot, &pe32))
            __leave;

        do
        {
            if (pe32.th32ProcessID == pid)
            {
                ppid = pe32.th32ParentProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    __finally
    {
        if (hSnapshot != INVALID_HANDLE_VALUE)
            CloseHandle(hSnapshot);
    }
    return ppid;
}

//------------------------------------------------------------------------------
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

    InitLogFile();

    std::stringstream ss;
    ss << "PID: " << ::_getpid() << ", Parent PID: " << ::getppid();
    LogDebug(ss.str());

    // Provide CEF with command-line arguments.
    LogDebug("Creating CefMainArgs");
    CefMainArgs main_args(hInstance);

    // SimpleApp implements application-level callbacks. It will create the
    // first browser instance in OnContextInitialized() after CEF has
    // initialized.
    LogDebug("Creating RenderProcess");
    CefRefPtr<RenderProcess> app(new RenderProcess);

    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
    // that share the same executable. This function checks the command-line
    // and, if this is a sub-process, executes the appropriate logic.
    //
    // IMPORTANT: This executable is ONLY meant to be launched by CEF as a
    // subprocess. CefExecuteProcess will handle everything and return a valid
    // exit code (>= 0). If it returns -1, it means this exe was launched
    // incorrectly (e.g., manually by the user).
    LogDebug("Calling CefExecuteProcess");
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);

    ss.str("");
    ss << "CefExecuteProcess returned exit_code: " << exit_code;
    LogDebug(ss.str());

    if (exit_code < 0)
    {
        // This should not happen - it means this executable was launched
        // directly instead of being spawned by CEF as a subprocess.
        LogDebug("ERROR: This executable should only be launched by CEF as a subprocess!");
        LogDebug("If you see this message, something is wrong with the CEF configuration.");
        exit_code = 1;
    }

    LogDebug("SubProcess exiting");
    if (g_log_file.is_open())
    {
        g_log_file.close();
    }
    return exit_code;
}