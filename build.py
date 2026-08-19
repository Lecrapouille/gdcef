#!/usr/bin/env python3
###############################################################################
## MIT License
##
## Copyright (c) 2022 Alain Duron <duron.alain@gmail.com>
## Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
###############################################################################
#
# This Python script is used to build a web viewer plugin based on the Chromium
# Embedded Framework (CEF) for 2D and 3D applications. This plugin is compatible
# with Godot 4.2, 4.3, Linux, and Windows.
# Edit the "Global user settings" section below to customize your build as needed.
#
# Note: If you are a Godot 3 user, you cloned the wrong branch :)
#
###############################################################################

import os, sys, subprocess, hashlib, tarfile, shutil, glob, progressbar, urllib.request
import importlib
from platform import machine, system
from pathlib import Path
from multiprocessing import cpu_count
from packaging import version
from shutil import copymode
import re

###############################################################################
#
# Global user settings.
# Edit this section to configure your build.
#
###############################################################################

# The fixed folder name that will contain all CEF build artifacts.
# (!) WARNING (!)
#  - Only specify a folder name, not a path.
#  - This folder name will also be used in the C++ part of this plugin to locate CEF prebuilt assets automatically.
CEF_ARTIFACTS_FOLDER_NAME = "cef_artifacts"

# CEF version to download from https://cef-builds.spotifycdn.com/index.html
# Copy the version listed there WITHOUT the OS or architecture; this script chooses the correct binary.
CEF_VERSION = "151.3.18+gbeff58d+chromium-151.0.7922.138"

# Your Godot editor version. Must match either:
#  - a "godot-<version>-stable" tag at https://github.com/godotengine/godot-cpp/tags
#  - or a "<version>" branch at https://github.com/godotengine/godot-cpp/branches
# (!) WARNING (!)
#  - Do not use version 4.1: gdextension is not compatible.
#  - Do not use version 3.x: use the godot-3.x branch of gdCEF instead.
GODOT_VERSION = "4.5"                                     # Example: "4.2" or a tag

# Use "godot-<version>-stable" for a tag, or "<version>" to track the HEAD of a branch.
GODOT_CPP_GIT_TAG_OR_BRANCH = GODOT_VERSION

# Build/compilation modes
COMPILATION_MODE = "release"                              # Or "debug"
CEF_TARGET = COMPILATION_MODE.title()                     # "Release" or "Debug" (first letter uppercase!)
GODOT_CPP_TARGET = "template_" + COMPILATION_MODE         # "template_release" or "template_debug"
MODULE_TARGET = COMPILATION_MODE                          # "release" or "debug"

# Enable OpenMP for multi-core processing (for copying CEF textures to Godot).
# Note: OpenMP is not installed by default on macOS :(
CEF_USE_CPU_PARALLELISM = "no"                            # Or "yes"

# macOS subprocess app bundle name (renamed from CEF's cefsimple.app after build).
MACOS_SUBPROCESS_APP = "gdCefRenderProcess.app"

# Minimum CMake version required to build CEF
CMAKE_MIN_VERSION = "3.19"

# SCons is the Godot build system. If the "scons" command is not found, you may need to fix your PATH
# (Windows: typically C:\Users\<username>\AppData\Local\Programs\Python\Python313\Scripts)
SCONS = "scons"                                           # Or ["python3", "-m", "SCons"]

###############################################################################
#
# Internal project paths. Do not modify.
#
###############################################################################
# Use the script's directory as the base for all other paths
script_dir = Path(__file__).resolve().parent
PWD = str(script_dir)
GDCEF_PATH = os.path.join(PWD, "gdcef", "browser")
GDCEF_PROCESSES_PATH = os.path.join(PWD, "gdcef", "subprocess")
GDCEF_THIRDPARTY_PATH = os.path.join(PWD, "thirdparty")
THIRDPARTY_CEF_PATH = os.path.join(GDCEF_THIRDPARTY_PATH, "cef_binary")
THIRDPARTY_GODOT_PATH = os.path.join(GDCEF_THIRDPARTY_PATH, "godot-" + GODOT_VERSION)
GODOT_CPP_API_PATH = os.path.join(THIRDPARTY_GODOT_PATH, "cpp")
PATCHES_PATH = os.path.join(PWD, "gdcef", "patches")
GDCEF_EXAMPLES_PATH = os.path.join(PWD, "demos")
GDCEF_TESTS_PATH = os.path.join(PWD, "gdcef", "tests")
CEF_ARTIFACTS_BUILD_PATH = str((script_dir / CEF_ARTIFACTS_FOLDER_NAME).resolve())

###############################################################################
#
# System/architecture detection
#
###############################################################################
ARCHI = machine()
if ARCHI == "AMD64":
    ARCHI = "x86_64"
NPROC = str(cpu_count())
OSTYPE = system()

# OS-specific subfolder name for artifacts
if OSTYPE == "Linux":
    OS_SUBDIR = "linux"
elif OSTYPE == "Darwin":
    OS_SUBDIR = "macos"
elif OSTYPE == "Windows" or OSTYPE == "MinGW":
    OS_SUBDIR = "windows"
else:
    OS_SUBDIR = "unknown"

# Full path to OS-specific artifacts folder
CEF_ARTIFACTS_OS_PATH = os.path.join(CEF_ARTIFACTS_BUILD_PATH, OS_SUBDIR)

###############################################################################
#
# Colored message helpers
#
###############################################################################
def info(msg):
    print("\033[32m[INFO] " + msg + "\033[00m", flush=True)

def warning(msg):
    print("\033[33m[WARNING] " + msg + "\033[00m", flush=True)

def fatal(msg):
    print("\033[31m[FATAL] " + msg + "\033[00m", flush=True)
    sys.exit(2)

###############################################################################
#
# Run a subprocess and capture stdout/stderr
#
###############################################################################
def exec(*args):
    command = list(args)
    try:
        result = subprocess.run(
            command,
            text=True,
            check=True
            # Output is displayed in real-time (not captured)
        )
        return result
    except subprocess.CalledProcessError as e:
        fatal(f"Failed executing: {' '.join(map(str, command))}")
    except FileNotFoundError:
        fatal(f"Command not found: {command[0]}")

###############################################################################
#
# Run SCons with parallel jobs
#
###############################################################################
def scons(*args):
    if isinstance(SCONS, str):
        exec(SCONS, *args, "--jobs=" + NPROC)
    else:
        exec(*SCONS, *args, "--jobs=" + NPROC)

###############################################################################
#
# Create a symlink, or copy if that's not possible (such as on Windows without admin privileges)
#
###############################################################################
def symlink(src, dst, force=False):
    try:
        p = Path(dst)
        if p.is_symlink():
            p.unlink()
        elif force and p.is_file():
            p.unlink()
        elif force and p.is_dir():
            shutil.rmtree(dst)

        os.symlink(src, dst)

    except OSError as e:
        # Common error: not enough privileges on Windows
        if hasattr(e, "winerror") and e.winerror == 1314:  # ERROR_PRIVILEGE_NOT_HELD
            warning("Cannot create symlink (requires administrator rights on Windows).")
            warning("Copying directory instead of creating symlink...")
            if os.path.isdir(src):
                shutil.copytree(src, dst, dirs_exist_ok=True)
            else:
                shutil.copy2(src, dst)
        else:
            raise

###############################################################################
#
# Verbose file copy: prints what it is copying
#
###############################################################################
def copyfile(file_name, folder):
    dest = os.path.join(folder, os.path.basename(file_name))
    print("Copy " + file_name + " => " + dest)
    shutil.copyfile(file_name, dest)
    copymode(file_name, dest)

###############################################################################
#
# Create a directory, like "mkdir -p"
#
###############################################################################
def mkdir(path):
    Path(path).mkdir(parents=True, exist_ok=True)

###############################################################################
#
# Remove a directory and all of its contents, like "rm -rf"
#
###############################################################################
def rmdir(top):
    if os.path.isdir(top):
        for root, dirs, files in os.walk(top, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        os.rmdir(top)

###############################################################################
#
# Extract a .tar.bz2 archive (like "tar -xj")
#
###############################################################################
def untarbz2(tar_bz2_file_name, dest_dir):
    info("Unpacking " + tar_bz2_file_name + " ...")
    temp_dir = dest_dir + "_temp"
    try:
        with tarfile.open(tar_bz2_file_name) as tar:
            tar.extractall(temp_dir)

        contents = os.listdir(temp_dir)

        # Move content out of a root folder if present
        if len(contents) == 1 and os.path.isdir(os.path.join(temp_dir, contents[0])):
            root_dir = os.path.join(temp_dir, contents[0])
            mkdir(dest_dir)
            for item in os.listdir(root_dir):
                src = os.path.join(root_dir, item)
                dst = os.path.join(dest_dir, item)
                print(" - %s" % dst)
                shutil.move(src, dst)
        else:
            mkdir(dest_dir)
            for item in contents:
                src = os.path.join(temp_dir, item)
                dst = os.path.join(dest_dir, item)
                print(" - %s" % dst)
                shutil.move(src, dst)
    finally:
        if os.path.exists(temp_dir):
            rmdir(temp_dir)

###############################################################################
#
# Search for a substring in a file (not a regex)
#
###############################################################################
def grep(file_name, what):
    try:
        with open(file_name, "r") as file:
            for line in file:
                if what in line:
                    return line
        return None
    except IOError:
        return None
    except Exception as e:
        warning(f"Error reading {file_name}: {e}")
        return None

###############################################################################
#
# Simple progress bar for urllib downloads
#
###############################################################################
class MyProgressBar():
    def __init__(self):
        self.pbar = None

    def __call__(self, block_num, block_size, total_size):
        if not self.pbar:
            self.pbar = progressbar.ProgressBar(maxval=total_size)
            self.pbar.start()

        downloaded = block_num * block_size
        if downloaded < total_size:
            self.pbar.update(downloaded)
        else:
            self.pbar.finish()

###############################################################################
#
# Download a file with a progress bar and warnings for large files
#
###############################################################################
def download(url, destination):
    if not url.startswith(('http://', 'https://')):
        fatal(f"Invalid URL scheme: {url}")

    info("Downloading " + url + " into " + destination)

    try:
        # 60 second timeout
        request = urllib.request.Request(url)
        with urllib.request.urlopen(request, timeout=60) as response:
            total_size = int(response.headers.get('content-length', 0))

            if total_size > 5 * 1024 * 1024 * 1024:  # 5 GB
                warning(f"File is very large: {total_size / (1024**3):.2f} GB")

            with open(destination, 'wb') as f:
                chunk_size = 8192
                downloaded = 0

                if total_size > 0:
                    pbar = progressbar.ProgressBar(maxval=total_size)
                    pbar.start()

                while True:
                    chunk = response.read(chunk_size)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)

                    if total_size > 0:
                        pbar.update(downloaded)

                if total_size > 0:
                    pbar.finish()

        print('', flush=True)

    except urllib.error.URLError as e:
        fatal(f"Download failed: {e}")
    except TimeoutError:
        fatal(f"Download timeout for {url}")
    except Exception as e:
        fatal(f"Unexpected error downloading {url}: {e}")

###############################################################################
#
# Compute SHA1 hash of a file
#
###############################################################################
def compute_sha1(artifact):
    CHUNK = 1 * 1024 * 1024
    sha1 = hashlib.sha1()
    with open(artifact, 'rb') as f:
        while True:
            data = f.read(CHUNK)
            if not data:
                break
            sha1.update(data)
    return "{0}".format(sha1.hexdigest())

###############################################################################
#
# Read the hash from a SHA1 file (only the first line)
#
###############################################################################
def read_sha1_file(path_sha1):
    with open(path_sha1, "r") as file:
        for line in file:
            return line.strip()
    return None

###############################################################################
#
# Basic path checks
#
###############################################################################
def check_paths():
    for path in [PWD, GDCEF_PATH, GDCEF_PROCESSES_PATH, GDCEF_EXAMPLES_PATH]:
        if not os.path.isdir(path):
            fatal('Folder ' + path + ' does not exist!')
    try:
        p = Path(CEF_ARTIFACTS_BUILD_PATH)
        if p.exists():
            if p.is_symlink():
                p.unlink()
            elif p.is_dir():
                shutil.rmtree(CEF_ARTIFACTS_BUILD_PATH)
            else:
                fatal('Please remove ' + CEF_ARTIFACTS_BUILD_PATH +
                      ' manually (it is not a directory or symlink), then re-run this script')
    except PermissionError:
        fatal(f'Permission denied while removing {CEF_ARTIFACTS_BUILD_PATH}')
    except Exception as e:
        fatal(f'Error handling {CEF_ARTIFACTS_BUILD_PATH}: {e}')

###############################################################################
#
# Download and extract Chromium Embedded Framework if not available locally
#
###############################################################################
def download_cef():
    if OSTYPE == "Linux":
        if ARCHI == "x86_64":
            CEF_ARCHI = "linux64"
        else:
            CEF_ARCHI = "linuxarm"
    elif OSTYPE == "Darwin":
        if ARCHI == "x86_64":
            CEF_ARCHI = "macosx64"
        else:
            CEF_ARCHI = "macosarm64"
    elif OSTYPE == "Windows":
        if ARCHI == "x86_64" or ARCHI == "AMD64":
            CEF_ARCHI = "windows64"
        else:
            CEF_ARCHI = "windowsarm64"
    else:
        fatal("Unknown OS/architecture " + OSTYPE + ": Cannot download Chromium Embedded Framework")

    # Check if the desired CEF version is already installed (by checking the README)
    if grep(os.path.join(THIRDPARTY_CEF_PATH, "README.txt"), CEF_VERSION) is not None:
        info(CEF_VERSION + " is already downloaded")
    else:
        CEF_URL_VERSION = CEF_VERSION.replace("+", "%2B")
        CEF_TARBALL = "cef_binary_" + CEF_URL_VERSION + "_" + CEF_ARCHI + ".tar.bz2"
        SHA1_CEF_TARBALL = CEF_TARBALL + ".sha1"
        info("Downloading Chromium Embedded Framework into " + THIRDPARTY_CEF_PATH + " ...")

        # Remove any preexisting or partially installed CEF artifacts
        mkdir(GDCEF_THIRDPARTY_PATH)
        os.chdir(GDCEF_THIRDPARTY_PATH)
        rmdir("cef_binary")

        URL = "https://cef-builds.spotifycdn.com/" + CEF_TARBALL
        info(URL)
        download(URL, CEF_TARBALL)
        download(URL + ".sha1", SHA1_CEF_TARBALL)

        if compute_sha1(CEF_TARBALL) != read_sha1_file(SHA1_CEF_TARBALL):
            os.remove(CEF_TARBALL)
            os.remove(SHA1_CEF_TARBALL)
            fatal("Downloaded CEF tarball does not match expected SHA1. Please retry!")

        # Extract to the canonical simple folder name
        untarbz2(CEF_TARBALL, THIRDPARTY_CEF_PATH)

        # Remove downloaded archives
        os.remove(CEF_TARBALL)
        os.remove(CEF_TARBALL + ".sha1")

###############################################################################
#
# Apply CEF build patches (mainly for Windows static build)
#
###############################################################################
def patch_cef():
    if os.path.isdir(THIRDPARTY_CEF_PATH):
        os.chdir(THIRDPARTY_CEF_PATH)
        info("Patching Chromium Embedded Framework")

        # Windows: patch CMakeLists for static build when used with Godot
        if OSTYPE == "Windows":
            shutil.copyfile(os.path.join(PATCHES_PATH, "CEF", "win", "libcef_dll_wrapper_cmake"),
                            "CMakeLists.txt")

###############################################################################
#
# Compile CEF (if needed)
#
###############################################################################
def compile_cef():
    if os.path.isdir(THIRDPARTY_CEF_PATH):
        patch_cef()
        os.chdir(THIRDPARTY_CEF_PATH)
        info("Compiling Chromium Embedded Framework in " + CEF_TARGET +
             " mode (inside " + THIRDPARTY_CEF_PATH + ") ...")

        if OSTYPE == "Windows":
            exec("cmake", "-DCEF_RUNTIME_LIBRARY_FLAG=/MD", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, ".")
            exec("cmake", "--build", ".", "--config", CEF_TARGET)
        elif OSTYPE == "Darwin":
            mkdir("build")
            os.chdir("build")
            exec("cmake", "-G", "Ninja", "-DPROJECT_ARCH=" + ARCHI, "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, "..")
            exec("ninja", "-v", "-j" + NPROC, "cefsimple")
        else:
            mkdir("build")
            os.chdir("build")
            if shutil.which('ninja') is not None:
                exec("cmake", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, "..")
                exec("ninja", "-v", "-j" + NPROC, "cefsimple")
            else:
                exec("cmake", "-G", "Unix Makefiles", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, "..")
                exec("make", "cefsimple", "-j" + NPROC)

###############################################################################
#
# Write version information file to artifact output directory
#
###############################################################################
def create_version_file():
    info("Creating VERSION file")
    try:
        with open(os.path.join(PWD, "VERSION"), "r") as f:
            gdcef_version = f.read().strip()
    except:
        warning("Could not read VERSION file")
        gdcef_version = "unknown"

    try:
        git_branch = subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"]).decode("utf-8").strip()
        git_sha1 = subprocess.check_output(["git", "rev-parse", "HEAD"]).decode("utf-8").strip()
    except:
        warning("Could not get git information")
        git_branch = "unknown"
        git_sha1 = "unknown"

    with open(os.path.join(CEF_ARTIFACTS_BUILD_PATH, "VERSION"), "w") as f:
        f.write("https://github.com/Lecrapouille/gdcef\n")
        f.write("gdCEF Version: " + gdcef_version + "\n")
        f.write("gdCEF Git Branch: " + git_branch + "\n")
        f.write("gdCEF Git SHA1: " + git_sha1 + "\n")
        f.write("CEF Version: " + CEF_VERSION + "\n")
        f.write("Godot Version: " + GODOT_VERSION + "\n")

###############################################################################
#
# Copy all CEF assets needed for your application to the output build folder
#
###############################################################################
def copy_cef_assets():
    # Create root artifacts folder and OS-specific subfolder
    mkdir(CEF_ARTIFACTS_BUILD_PATH)
    mkdir(CEF_ARTIFACTS_OS_PATH)

    # Copy all CEF artifacts required for the application.
    # Note: We do not copy chrome-sandbox, it is not required for GDCEF.
    info("Installing Chromium Embedded Framework to " + CEF_ARTIFACTS_OS_PATH + " ...")

    # Locales stay at the root level (shared across OS)
    locales = os.path.join(CEF_ARTIFACTS_OS_PATH, "locales")
    mkdir(locales)

    if OSTYPE == "Linux" or OSTYPE == "Windows":
        S = os.path.join(THIRDPARTY_CEF_PATH, "Resources")
        # OS-specific files go to OS subfolder
        copyfile(os.path.join(S, "icudtl.dat"), CEF_ARTIFACTS_OS_PATH)
        for f in glob.glob(os.path.join(S, "*.pak")):
            copyfile(f, CEF_ARTIFACTS_OS_PATH)
        # Locales stay at root
        for f in glob.glob(os.path.join(S, "locales/*")):
            copyfile(f, locales)

        S = os.path.join(THIRDPARTY_CEF_PATH, CEF_TARGET)
        copyfile(os.path.join(S, "vk_swiftshader_icd.json"), CEF_ARTIFACTS_OS_PATH)
        for f in glob.glob(os.path.join(S, "*snapshot*.bin")):
            copyfile(f, CEF_ARTIFACTS_OS_PATH)
        if OSTYPE == "Linux":
            for f in glob.glob(os.path.join(S, "*.so")):
                copyfile(f, CEF_ARTIFACTS_OS_PATH)
            for f in glob.glob(os.path.join(S, "*.so.*")):
                copyfile(f, CEF_ARTIFACTS_OS_PATH)
        else:
            for f in glob.glob(os.path.join(S, "*.dll")):
                copyfile(f, CEF_ARTIFACTS_OS_PATH)
    elif OSTYPE == "Darwin":
        S = os.path.join(THIRDPARTY_CEF_PATH, "build", "tests", "cefsimple", CEF_TARGET, "cefsimple.app")
        macos_app_path = os.path.join(CEF_ARTIFACTS_OS_PATH, MACOS_SUBPROCESS_APP)
        shutil.copytree(S, macos_app_path)
        for f in glob.glob(os.path.join(S, "locales/*")):
            copyfile(f, locales)
    else:
        fatal("Unknown OS " + OSTYPE + ": Cannot extract CEF artifacts")

###############################################################################
#
# Install gdCefRenderProcess into the macOS app bundle Helper executables
#
###############################################################################
def install_gdcef_render_process_macos():
    render_process = os.path.join(CEF_ARTIFACTS_OS_PATH, "gdCefRenderProcess")
    if not os.path.isfile(render_process):
        fatal("gdCefRenderProcess binary not found at " + render_process)

    app_path = os.path.join(CEF_ARTIFACTS_OS_PATH, MACOS_SUBPROCESS_APP)
    frameworks = os.path.join(app_path, "Contents", "Frameworks")
    if not os.path.isdir(frameworks):
        fatal(MACOS_SUBPROCESS_APP + " bundle is missing Contents/Frameworks at " + frameworks)

    helper_count = 0
    for helper_app in glob.glob(os.path.join(frameworks, "* Helper*.app")):
        macos_dir = os.path.join(helper_app, "Contents", "MacOS")
        if not os.path.isdir(macos_dir):
            continue
        for helper_exe in glob.glob(os.path.join(macos_dir, "*")):
            if not os.path.isfile(helper_exe):
                continue
            info("Installing gdCefRenderProcess into " + helper_exe)
            shutil.copy2(render_process, helper_exe)
            os.chmod(helper_exe, 0o755)
            helper_count += 1

    if helper_count == 0:
        fatal("No Helper executables found under " + frameworks)

    os.remove(render_process)
    info("Removed standalone gdCefRenderProcess (subprocess runs from " + MACOS_SUBPROCESS_APP + ")")

###############################################################################
#
# Download the Godot-cpp wrapper needed for gdnative CEF modules
#
###############################################################################
def download_godot_cpp():
    if not os.path.exists(GODOT_CPP_API_PATH):
        info("Cloning Godot C++ wrapper (" + GODOT_VERSION + ") into " + GODOT_CPP_API_PATH)
        mkdir(GODOT_CPP_API_PATH)
        exec("git", "ls-remote", "https://github.com/godotengine/godot-cpp", GODOT_CPP_GIT_TAG_OR_BRANCH)
        exec("git", "clone", "--recursive", "-b", GODOT_CPP_GIT_TAG_OR_BRANCH,
             "https://github.com/godotengine/godot-cpp", GODOT_CPP_API_PATH)

###############################################################################
#
# Compile godot-cpp if needed
#
###############################################################################
def compile_godot_cpp():
    lib = os.path.join(GODOT_CPP_API_PATH, "bin", "libgodot-cpp*" + GODOT_CPP_TARGET + "*")
    if not os.path.exists(lib):
        info("Compiling Godot C++ API (inside " + GODOT_CPP_API_PATH + ") ...")
        os.chdir(GODOT_CPP_API_PATH)
        if OSTYPE == "Linux":
            scons("platform=linux",
                  "target=" + GODOT_CPP_TARGET,
                  "use_static_cpp=no")
        elif OSTYPE == "Darwin":
            scons("platform=macos",
                  "arch=" + ARCHI,
                  "target=" + GODOT_CPP_TARGET,
                  "use_static_cpp=no")
        elif OSTYPE == "MinGW":
            scons("platform=windows",
                  "use_mingw=True",
                  "target=" + GODOT_CPP_TARGET,
                  "use_static_cpp=no")
        elif OSTYPE == "Windows":
            scons("platform=windows",
                  "target=" + GODOT_CPP_TARGET,
                  "use_static_cpp=no")
        else:
            fatal("Unknown OS " + OSTYPE + ": Cannot compile godot-cpp")

###############################################################################
#
# Helper: build arguments for scons command for building GDNative modules
#
###############################################################################
def gdnative_scons_cmd(platform):
    scons("api_path=" + GODOT_CPP_API_PATH,
          "cef_artifacts_folder=\\\"" + CEF_ARTIFACTS_FOLDER_NAME + "\\\"",
          "build_path=" + CEF_ARTIFACTS_OS_PATH,
          "target=" + MODULE_TARGET,
          "platform=" + platform,
          "arch=" + ARCHI,
          "cpu_parallelism=" + CEF_USE_CPU_PARALLELISM)

###############################################################################
#
# Compile the Godot CEF module (primary and subprocess)
#
###############################################################################
def compile_gdnative_cef(path):
    info("Compiling Godot CEF module in " + path)
    os.chdir(path)
    if OSTYPE == "Linux":
        gdnative_scons_cmd("x11")
    elif OSTYPE == "Darwin":
        gdnative_scons_cmd("macos")
    elif OSTYPE == "Windows" or OSTYPE == "MinGW":
        gdnative_scons_cmd("windows")
    else:
        fatal("Unknown OS " + OSTYPE + ": Cannot compile CEF module")

###############################################################################
#
# Create Godot .gdextension file with artifact folder name injected
#
###############################################################################
def create_gdextension_file():
    info("Creating Godot .gdextension file")
    with open(os.path.join(GDCEF_PATH, "gdcef.gdextension.in"), "r") as f:
        extension = f.read().replace("CEF_ARTIFACTS_FOLDER_NAME", CEF_ARTIFACTS_FOLDER_NAME)
    with open(os.path.join(CEF_ARTIFACTS_BUILD_PATH, "gdcef.gdextension"), "w") as f:
        f.write(extension)

###############################################################################
#
# Test if compilers are present (Windows)
#
###############################################################################
def check_compiler():
    if OSTYPE == "Windows":
        cppfile = "win.cc"
        binfile = "win.exe"
        objfile = "win.obj"
        with open(cppfile, "w") as f:
            f.write("#include <windows.h>\n")
            f.write("int main(int argc, char **argv) { return 0; }")
        if os.system("cl.exe /Fe:" + binfile + " " + cppfile) != 0:
            os.remove(cppfile)
            fatal("MS C++ compiler is not found. "
                  "Please install Visual Studio (https://visualstudio.microsoft.com) "
                  "and open an x64 Native Tools Command Prompt for VS 2022 with Administrator privileges.")
        if not os.path.isfile(binfile):
            os.remove(cppfile)
            fatal("MS C++ compiler is not working. "
                  "Please install Visual Studio (https://visualstudio.microsoft.com) "
                  "and open an x64 Native Tools Command Prompt for VS 2022 with Administrator privileges.")
        if os.system(os.path.join(".", binfile)) != 0:
            os.remove(cppfile)
            fatal("MS C++ compiler failed to build a test program. "
                  "Please install Visual Studio (https://visualstudio.microsoft.com) "
                  "and open an x64 Native Tools Command Prompt for VS 2022 with Administrator privileges.")
        info("MS C++ Compiler OK")
        os.remove(cppfile)
        os.remove(binfile)
        os.remove(objfile)

###############################################################################
#
# Make sure CMake is installed and meets minimum version
#
###############################################################################
def check_cmake_version():
    DOC_URL = "https://github.com/stigmee/doc-internal/blob/master/doc/install_latest_cmake.sh"
    info("Checking CMake version ...")
    if shutil.which("cmake") is None:
        fatal("It appears CMake is not installed. For Linux see " + DOC_URL +
              " to update it before running this script. For Windows, install "
              "the latest installer.")
    output = subprocess.check_output(["cmake", "--version"]).decode("utf-8")
    line = output.splitlines()[0]
    current_version = line.split()[2].split('-')[0]
    if version.parse(current_version) < version.parse(CMAKE_MIN_VERSION):
        fatal("Your CMake version is " + current_version + " but must be >= "
              + CMAKE_MIN_VERSION + ".\nSee " + DOC_URL + " to update it before "
              "running this script on Linux. For Windows, install the latest installer.")

###############################################################################
#
# Check that all required build tools are installed
#
###############################################################################
def check_build_chain():
    info("Checking if the build chain is present")
    if not shutil.which('cmake'):
        fatal("You need to install the 'cmake' tool")
    if not (shutil.which('ninja') or shutil.which('make')):
        fatal("You need to install either the 'ninja' or GNU Make tool")
    if (shutil.which('ninja') is None and OSTYPE == "Darwin"):
        fatal("You need to install the 'ninja' tool for macOS builds")
    if isinstance(SCONS, str):
        if not shutil.which(SCONS):
            fatal("You need to install the 'scons' tool")
    elif importlib.util.find_spec("scons") is None:
        fatal("You need to install the 'scons' tool")
    check_cmake_version()
    check_compiler()

###############################################################################
#
# On Windows, require that the script is run with administrator rights
#
###############################################################################
def check_run_as_windows_administrator():
    if OSTYPE == "Windows":
        import ctypes
        if not ctypes.windll.shell32.IsUserAnAdmin():
            fatal("You must run this script with administrator privileges")

###############################################################################
#
# We have multiple demos, and CEF artifacts are large (>1GB).
# To conserve disk space, we use symlinks in demo directories to point to the actual artifact folder.
# On Windows, creating symlinks requires administrator rights. If unavailable, fall back to copying.
#
###############################################################################
def copy_gdcef_artifacts(folder_paths):
    info("Setting up gdCEF artifacts for Godot demos and tests:")
    for folder_path in folder_paths:
        for filename in os.listdir(folder_path):
            path = os.path.join(folder_path, filename)
            if os.path.isdir(path) and os.path.isfile(os.path.join(path, "project.godot")):
                info("  - Demo " + path)
                artifacts_path = os.path.join(path, CEF_ARTIFACTS_FOLDER_NAME)
                # Remove existing folder/symlink and create new symlink
                # On Windows without administrator rights: fall back to copying the artifact folder.
                symlink(CEF_ARTIFACTS_BUILD_PATH, artifacts_path, force=True)

###############################################################################
#
# Print instructions at the end of build
#
###############################################################################
def final_instructions():
    print("")
    info("Build completed successfully!\n\n"
         "You can now open your Godot editor version " + GODOT_VERSION + " and try one of the demos located in '" + GDCEF_EXAMPLES_PATH + "'.\n"
         "All CEF and Godot artifacts have been generated in '" + CEF_ARTIFACTS_BUILD_PATH + "'.\n"
         "You can use this folder directly in your own Godot project by copying it.\n"
         "Note: To use a different artifact folder name, edit the value of CEF_ARTIFACTS_FOLDER_NAME in build.py and re-run the script.\n"
         "Note: In the demos, '" + CEF_ARTIFACTS_FOLDER_NAME + "' is a symlink not an actual folder. We use symlinks to save disk space,\n"
         "as the artifacts are large (>1GB), to avoid duplicating them for each demo. For your final application, copying the folder directly is preferred.\n")
    info("Have fun! :)\n\n")

###############################################################################
#
# Clone additional demo repositories listed in a text file
#
###############################################################################
def clone_github_projects():
    repos_file = os.path.join(GDCEF_EXAMPLES_PATH, "repos.txt")
    info("Cloning additional GitHub demo projects listed in " + repos_file)

    if not os.path.exists(repos_file):
        warning("The file " + repos_file + " does not exist. No additional demo projects will be cloned.")
        return

    with open(repos_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            match = re.search(r'github.com/[^/]+/([^/]+)', line)
            if not match:
                warning("Invalid GitHub URL: " + line)
                continue

            repo_name = match.group(1)
            # Remove extension if present (e.g., ".git")
            repo_name = repo_name.split('.')[0]
            repo_path = os.path.join(GDCEF_EXAMPLES_PATH, repo_name)

            if os.path.exists(repo_path):
                info("Repository " + repo_name + " already exists at " + repo_path)
                continue

            try:
                info("Cloning " + line + " into " + repo_path)
                exec("git", "clone", "--recursive", line, repo_path)
            except Exception as e:
                warning("Error while cloning " + line + ": " + str(e))

###############################################################################
#
# Entry point
#
###############################################################################
if __name__ == "__main__":
    check_run_as_windows_administrator()
    check_paths()
    check_build_chain()
    download_godot_cpp()
    compile_godot_cpp()
    download_cef()
    compile_cef()
    copy_cef_assets()
    create_version_file()
    compile_gdnative_cef(GDCEF_PATH)
    compile_gdnative_cef(GDCEF_PROCESSES_PATH)
    if OSTYPE == "Darwin":
        install_gdcef_render_process_macos()
    create_gdextension_file()
    clone_github_projects()
    copy_gdcef_artifacts([GDCEF_EXAMPLES_PATH, GDCEF_TESTS_PATH])
    final_instructions()
