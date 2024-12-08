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
###
### This python script allows to compile a web viewer plugin based on Chromium
### Embedded Framework (CEF) needed for your 2D and 3D applications. This plugin
### works for Godot 4.2, 4.3, Linux and Windows. Do not hesitate to edit the
### section "Global user settings" of this script to make a custom build.
###
### Note: if you are a Godot-3 user, you have git cloned the wrong branch :)
###
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
###
### Global user settings.
### Please edit this section to customize your build.
###
###############################################################################

# If set, then download prebuilt GDCEF artifacts instead of compiling from
# code source. See https://github.com/Lecrapouille/gdcef/releases to get the
# desired version (without 'v' and without godot version). You cannot choose
# neither Godot version not CEF version.
# If unset, then compile GCEF sources.
GITHUB_GDCEF_RELEASE = None                              # or "0.14.0"

# The hard-coded name of the folder that will hold all CEF built artifacts.
# /!\ BEWARE /!\
#  - Do not give a path but just a folder name.
#  - The folder name will used inside the C++ code of this plugin for finding
#    automatically CEF prebuilt stuffs.
CEF_ARTIFACTS_FOLDER_NAME = "cef_artifacts"

# CEF version downloaded from https://cef-builds.spotifycdn.com/index.html
# Copy-paste the version given on the web page WITHOUT the operation system or
# architecture since this script is enough smart to download the correct version :)
CEF_VERSION = "131.3.1+gcb062df+chromium-131.0.6778.109"

# Version of your Godot editor that shall match either:
#  - a "godot-<version>-stable" tag on https://github.com/godotengine/godot-cpp/tags
#  - or a "<version>"" branch on https://github.com/godotengine/godot-cpp/branches
# /!\ BEWARE /!\
#  - Do not use version 4.1 since gdextension is not compatible.
#  - Do not use version 3.x since not compatible. git checkout godot-3.x the gdcef branch instead.
GODOT_VERSION = "4.3"                                     # or "4.2" or tag

# Use "godot-<version>-stable" for a tag on https://github.com/godotengine/godot-cpp/tags
# Else "<version>" to track the HEAD of a branch https://github.com/godotengine/godot-cpp/branches
GODOT_CPP_GIT_TAG_OR_BRANCH = GODOT_VERSION               # or "godot-" + GODOT_VERSION + "-stable"

# Compilation mode
COMPILATION_MODE = "release"                              # or "debug"
# Compilation mode for the thirdpart CEF
CEF_TARGET = COMPILATION_MODE.title()                     # "Release" or "Debug" (with upper 1st letter !!!)
# Compilation mode for the thirdpart godot-cpp
GODOT_CPP_TARGET = "template_" + COMPILATION_MODE         # "template_release" or "template_debug"
# Compilation mode for gdcef as Godot module
MODULE_TARGET = COMPILATION_MODE                          # "release" or "debug"

# Use OpenMP for using CPU parallelism (i.e. for copying CEF textures to Godot)
# FIXME no openmp installed by default on MacOS :(
CEF_USE_CPU_PARALLELISM = "yes"                           # or "no"

# Minimun CMake version needed for compiling CEF
CMAKE_MIN_VERSION = "3.19"

# Scons is the build system used by Godot. For some people "scons" command is not recognized.
# I guess they are Windows users and they have not set correctly their PATH variable. (i.e.
# C:\Users\<username>\AppData\Local\Programs\Python\Python313\Scripts)
SCONS = "scons"                                           # or ["python3", "-m", "SCons"]

###############################################################################
###
### Project internal paths local from this script. Do not change them!
###
###############################################################################
PWD = os.getcwd()
GDCEF_PATH = os.path.join(PWD, "gdcef")
GDCEF_PROCESSES_PATH = os.path.join(PWD, "render_process")
GDCEF_THIRDPARTY_PATH = os.path.join(PWD, "thirdparty")
THIRDPARTY_CEF_PATH = os.path.join(GDCEF_THIRDPARTY_PATH, "cef_binary")
THIRDPARTY_GODOT_PATH = os.path.join(GDCEF_THIRDPARTY_PATH, "godot-" + GODOT_VERSION)
GODOT_CPP_API_PATH = os.path.join(THIRDPARTY_GODOT_PATH, "cpp")
PATCHES_PATH = os.path.join(PWD, "patches")
GDCEF_EXAMPLES_PATH = os.path.join(PWD, "demos")
CEF_ARTIFACTS_BUILD_PATH = os.path.realpath(os.path.join("../../" + CEF_ARTIFACTS_FOLDER_NAME))

###############################################################################
###
### Type of operating system, AMD64, ARM64 ...
###
###############################################################################
ARCHI = machine()
if ARCHI == "AMD64":
    ARCHI = "x86_64"
NPROC = str(cpu_count())
OSTYPE = system()

###############################################################################
###
### Green color message
###
###############################################################################
def info(msg):
    print("\033[32m[INFO] " + msg + "\033[00m", flush=True)

###############################################################################
###
### Orange color message
###
###############################################################################
def warning(msg):
    print("\033[33m[WARNING] " + msg + "\033[00m", flush=True)

###############################################################################
###
### Red color message + abort
###
###############################################################################
def fatal(msg):
    print("\033[31m[FATAL] " + msg + "\033[00m", flush=True)
    sys.exit(2)

###############################################################################
###
### Wrap the subprocess command
###
###############################################################################
def exec(*args):
    command = list(args)
    try:
        subprocess.run(command, text=True, check=True)
    except subprocess.CalledProcessError as e:
        fatal("Failed executing the command " + ' '.join(map(str,command)))

###############################################################################
###
### Wrap the scons command
###
###############################################################################
def scons(*args):
    if type(SCONS) == str:
        exec(SCONS, *args, "--jobs=" + NPROC)
    else:
        exec(*SCONS, *args, "--jobs=" + NPROC)

###############################################################################
###
### Equivalent to test -L e on alias + ln -s
###
###############################################################################
def symlink(src, dst, force=False):
    p = Path(dst)
    if p.is_symlink():
        os.remove(p)
    elif force and p.is_file():
        os.remove(p)
    elif force and p.is_dir():
        rmdir(dst)
    os.symlink(src, dst)

###############################################################################
###
### Equivalent to cp --verbose
###
###############################################################################
def copyfile(file_name, folder):
    dest = os.path.join(folder, os.path.basename(file_name))
    print("Copy " + file_name + " => " + dest)
    shutil.copyfile(file_name, dest)
    copymode(file_name, dest)

###############################################################################
###
### Equivalent to mkdir -p
###
###############################################################################
def mkdir(path):
    Path(path).mkdir(parents=True, exist_ok=True)

###############################################################################
###
### Equivalent to rm -fr
###
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
###
### Equivalent to tar -xj
###
###############################################################################
def untarbz2(tar_bz2_file_name, dest_dir):
    info("Unpacking " + tar_bz2_file_name + " ...")
    with tarfile.open(tar_bz2_file_name) as f:
        root_dir = ""
        for tarinfo in f:
            if tarinfo.isdir() and root_dir == "":
                root_dir = tarinfo.name
            name = tarinfo.name.replace(root_dir, dest_dir)
            print(" - %s" % name)
            if tarinfo.isdir():
                mkdir(name)
                continue
            tarinfo.name = name
            f.extract(tarinfo, "")

###############################################################################
###
### Search an expression (not a regexp) inside a file
###
###############################################################################
def grep(file_name, what):
    try:
        file = open(file_name, "r")
        for line in file:
            if line.find(what) != -1:
                return line
        return None
    except IOError:
        return None

###############################################################################
###
### Needed for urllib.request.urlretrieve
### See https://stackoverflow.com/a/53643011/8877076
###
###############################################################################
class MyProgressBar():
    def __init__(self):
        self.pbar = None

    def __call__(self, block_num, block_size, total_size):
        if not self.pbar:
            self.pbar=progressbar.ProgressBar(maxval=total_size)
            self.pbar.start()

        downloaded = block_num * block_size
        if downloaded < total_size:
            self.pbar.update(downloaded)
        else:
            self.pbar.finish()

###############################################################################
###
### Download artifacts
###
###############################################################################
def download(url, destination):
    info("Download " + url + " into " + destination)
    urllib.request.urlretrieve(url, destination, reporthook=MyProgressBar())
    print('', flush=True)

###############################################################################
###
### Compute the SHA1 of the given artifact file
###
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
###
### Read a text file holding a SHA1 value
###
###############################################################################
def read_sha1_file(path_sha1):
    file = open(path_sha1, "r")
    for line in file:
        return line # Just read 1st line
    return None

###############################################################################
###
### Give some path checks
###
###############################################################################
def check_paths():
    for path in [PWD, GDCEF_PATH, GDCEF_PROCESSES_PATH, GDCEF_EXAMPLES_PATH]:
        if not os.path.isdir(path):
            fatal('Folder ' + path + ' does not exist!')

    # Remove the example build folder to avoid messed up with your
    # application build using alias.
    p = Path(CEF_ARTIFACTS_BUILD_PATH);
    if p.is_symlink():
        os.remove(CEF_ARTIFACTS_BUILD_PATH)
    elif p.is_dir():
        rmdir(CEF_ARTIFACTS_BUILD_PATH)
    elif p.exists():
        fatal('Please remove manually ' + CEF_ARTIFACTS_BUILD_PATH + ' and recall this script')

###############################################################################
###
### Download prebuild CEF artifacts from GitHub releases
###
###############################################################################
def download_gdcef_release():
    info("Download prebuilt GDCEF artifacts from GitHub instead of compiling sources")
    if not ((OSTYPE == "Linux" or OSTYPE == "Windows") and (ARCHI == "x86_64")):
        fatal("OS " + OSTYPE + " architecture " + ARCHI + " is not available as GitHub release")

    GITHUB_URL = "https://github.com/Lecrapouille/gdcef/releases/download/"
    RELEASE_TAG = "v" + GITHUB_GDCEF_RELEASE + "-godot" + GODOT_VERSION[0] + "/"

    # Tarball name format depends on the release version
    version_num = version.parse(GITHUB_GDCEF_RELEASE)
    version_0_13 = version.parse("0.13.0")
    if version_num < version_0_13:
        TARBALL_NAME = "gdcef-artifacts-godot_" + GODOT_VERSION[0] + "-" + OSTYPE.lower() + "_" + ARCHI + ".tar.gz"
    else: # New format for versions >= 0.13.0
        arch_str = "ARM64" if OSTYPE == "Darwin" else "X64"
        os_str = ""
        if OSTYPE == "Linux":
            os_str = "Linux"
        elif OSTYPE == "Windows":
            os_str = "Windows"
        elif OSTYPE == "Darwin":
            os_str = "macOS"
        TARBALL_NAME = "gdCEF-" + GITHUB_GDCEF_RELEASE + "_Godot-" + GODOT_VERSION + "_" + os_str + "_" + arch_str + ".tar.gz"

    URL = GITHUB_URL + RELEASE_TAG + TARBALL_NAME

    try:
        download(URL, TARBALL_NAME)
        untarbz2(TARBALL_NAME, CEF_ARTIFACTS_BUILD_PATH)
        os.remove(TARBALL_NAME)
    except Exception as err:
        fatal(URL + " does not exist. Are you sure of the desired version ? Else try to compile GDCEF")

###############################################################################
###
### Download prebuild Chromium Embedded Framework if folder is not present
###
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

    # CEF already installed ? Installed with a different version ?
    # Compare the desired CEF version (to be downloaded) with the potentially
    # installed CEF. The version is stored in the README and if not present
    # or not matching that means the CEF shall be downloaded as "cef_binary" folder.
    if grep(os.path.join(THIRDPARTY_CEF_PATH, "README.txt"), CEF_VERSION) != None:
        info(CEF_VERSION + " already downloaded")
    else:
        # Replace the '+' chars by URL percent encoding '%2B'
        CEF_URL_VERSION = CEF_VERSION.replace("+", "%2B")
        CEF_TARBALL = "cef_binary_" + CEF_URL_VERSION + "_" + CEF_ARCHI + ".tar.bz2"
        SHA1_CEF_TARBALL = CEF_TARBALL + ".sha1"
        info("Downloading Chromium Embedded Framework into " + THIRDPARTY_CEF_PATH + " ...")

        # Remove the CEF folder if existing or partially downloaded/compiled.
        mkdir(GDCEF_THIRDPARTY_PATH)
        os.chdir(GDCEF_THIRDPARTY_PATH)
        rmdir("cef_binary")

        # Download CEF at https://cef-builds.spotifycdn.com/index.html
        URL = "https://cef-builds.spotifycdn.com/" + CEF_TARBALL
        info(URL)
        download(URL, CEF_TARBALL)
        download(URL + ".sha1", SHA1_CEF_TARBALL)
        if compute_sha1(CEF_TARBALL) != read_sha1_file(SHA1_CEF_TARBALL):
            os.remove(CEF_TARBALL)
            os.remove(SHA1_CEF_TARBALL)
            fatal("Downloaded CEF tarball does not match expected SHA1. Please retry!")

        # Simplify the folder name by removing the complex version number
        untarbz2(CEF_TARBALL, THIRDPARTY_CEF_PATH)

        # Remove useless files
        os.remove(CEF_TARBALL)
        os.remove(CEF_TARBALL + ".sha1")

###############################################################################
###
### Patch Chromium Embedded Framework for Windows, if not already made by this
### script previously.
###
###############################################################################
def patch_cef():
    if os.path.isdir(THIRDPARTY_CEF_PATH):
        os.chdir(THIRDPARTY_CEF_PATH)
        info("Patching Chromium Embedded Framework")

        # Apply patches for Windows for compiling as static lib. This is needed
        # for being used with Godot.
        if OSTYPE == "Windows":
            shutil.copyfile(os.path.join(PATCHES_PATH, "CEF", "win", "libcef_dll_wrapper_cmake"),
                            "CMakeLists.txt")

###############################################################################
###
### Compile Chromium Embedded Framework if not already made by this script
### previously.
###
###############################################################################
def compile_cef():
    if os.path.isdir(THIRDPARTY_CEF_PATH):
        patch_cef()

        os.chdir(THIRDPARTY_CEF_PATH)
        info("Compiling Chromium Embedded Framework in " + CEF_TARGET +
             " mode (inside " + THIRDPARTY_CEF_PATH + ") ...")

        if OSTYPE == "Windows":
            # Windows: force compiling CEF as static library. This is needed
            # for being used with Godot.
            exec("cmake", "-DCEF_RUNTIME_LIBRARY_FLAG=/MD", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, ".")
            exec("cmake", "--build", ".", "--config", CEF_TARGET)
        elif OSTYPE == "Darwin":
           # MacOS: Compile CEF using Ninja
           mkdir("build")
           os.chdir("build")
           exec("cmake", "-G", "Ninja", "-DPROJECT_ARCH="+ARCHI, "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, "..")
           exec("ninja", "-v", "-j" + NPROC, "cefsimple")
        else:
           # Linux: Compile CEF if Ninja is available else use default
           # GNU Makefile.
           mkdir("build")
           os.chdir("build")
           if shutil.which('ninja') != None:
               exec("cmake", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, "..")
               exec("ninja", "-v", "-j" + NPROC, "cefsimple")
           else:
               exec("cmake", "-G", "Unix Makefiles", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, "..")
               exec("make", "cefsimple", "-j" + NPROC)

###############################################################################
###
### Create version information file
###
###############################################################################
def create_version_file():
    info("Creating GDCEF_VERSION.txt file")
    try:
        # Try to read version from VERSION file
        with open(os.path.join("..", PWD, "VERSION"), "r") as f:
            gdcef_version = f.read().strip()
    except:
        # Else try to read version from plugin.cfg
        try:
            with open(os.path.join(PWD, "plugin.cfg"), "r") as f:
                for line in f:
                    if line.startswith("version="):
                        gdcef_version = line.split("=")[1].strip().strip('"')
                        break
        except:
            warning("Could not read VERSION file nor plugin.cfg")
            gdcef_version = "unknown"

    # Get git information
    try:
        git_branch = subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"]).decode("utf-8").strip()
        git_sha1 = subprocess.check_output(["git", "rev-parse", "HEAD"]).decode("utf-8").strip()
    except:
        warning("Could not get git information")
        git_branch = "unknown"
        git_sha1 = "unknown"

    with open(os.path.join(CEF_ARTIFACTS_BUILD_PATH, "GDCEF_VERSION.txt"), "w") as f:
        f.write("gdCEF Version: " + gdcef_version + "\n")
        f.write("gdCEF Git Branch: " + git_branch + "\n")
        f.write("gdCEF Git SHA1: " + git_sha1 + "\n")
        f.write("CEF Version: " + CEF_VERSION + "\n")
        f.write("Godot Version: " + GODOT_VERSION + "\n")

###############################################################################
###
### Copy Chromium Embedded Framework assets to your application build folder
###
###############################################################################
def copy_cef_assets():
    build_path = CEF_ARTIFACTS_BUILD_PATH
    mkdir(build_path)

    ### Get all CEF compiled artifacts needed for your application.
    ### Note: We do not copy the chrome-sandbox since it is not needed for GDCEF.
    info("Installing Chromium Embedded Framework to " + build_path + " ...")
    locales = os.path.join(build_path, "locales")
    mkdir(locales)
    if OSTYPE == "Linux" or OSTYPE == "Windows":
        # gdcef/addons/gdcef/thirdparty/cef_binary/Resources
        S = os.path.join(THIRDPARTY_CEF_PATH, "Resources")
        copyfile(os.path.join(S, "icudtl.dat"), build_path)
        for f in glob.glob(os.path.join(S, "*.pak")):
            copyfile(f, build_path)
        for f in glob.glob(os.path.join(S, "locales/*")):
            copyfile(f, locales)

        # Either: gdcef/addons/gdcef/thirdparty/cef_binary/Release
        # or:     gdcef/addons/gdcef/thirdparty/cef_binary/Debug
        S = os.path.join(THIRDPARTY_CEF_PATH, CEF_TARGET)
        copyfile(os.path.join(S, "vk_swiftshader_icd.json"), build_path)
        for f in glob.glob(os.path.join(S, "*snapshot*.bin")):
            copyfile(f, build_path)
        if OSTYPE == "Linux":
            for f in glob.glob(os.path.join(S, "*.so")):
                copyfile(f, build_path)
            for f in glob.glob(os.path.join(S, "*.so.*")):
                copyfile(f, build_path)
        else:
            for f in glob.glob(os.path.join(S, "*.dll")):
                copyfile(f, build_path)
    elif OSTYPE == "Darwin":
        S = os.path.join(THIRDPARTY_CEF_PATH, "build", "tests", "cefsimple", CEF_TARGET, "cefsimple.app")
        shutil.copytree(S, build_path + "/cefsimple.app")
        for f in glob.glob(os.path.join(S, "locales/*")):
            copyfile(f, locales)
    else:
        fatal("Unknown architecture " + OSTYPE + ": I dunno how to extract CEF artifacts")

###############################################################################
###
### Download Godot cpp wrapper needed for our gdnative code: CEF ...
###
###############################################################################
def download_godot_cpp():
    if not os.path.exists(GODOT_CPP_API_PATH):
        info("Clone cpp wrapper for Godot " + GODOT_VERSION + " into " + GODOT_CPP_API_PATH)
        mkdir(GODOT_CPP_API_PATH)
        exec("git", "ls-remote", "https://github.com/godotengine/godot-cpp", GODOT_CPP_GIT_TAG_OR_BRANCH)
        exec("git", "clone", "--recursive", "-b", GODOT_CPP_GIT_TAG_OR_BRANCH,
             "https://github.com/godotengine/godot-cpp", GODOT_CPP_API_PATH)

###############################################################################
###
### Compile Godot cpp wrapper needed for our gdnative code: CEF ...
###
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
            fatal("Unknown architecture " + OSTYPE + ": I dunno how to compile Godot-cpp")

###############################################################################
###
### Common Scons command for compiling our Godot gdnative modules
###
###############################################################################
def gdnative_scons_cmd(platform):
    use_openmp = CEF_USE_CPU_PARALLELISM
    # FIXME no openmp installed by default on MacOS :(
    # https://gist.github.com/ijleesw/4f863543a50294e3ba54acf588a4a421
    if OSTYPE == "Darwin":
        warning("Sorry for MacOS I have to disable openmp !!!")
        use_openmp = "no"
    scons("api_path=" + GODOT_CPP_API_PATH,
          "cef_artifacts_folder=\\\"" + CEF_ARTIFACTS_FOLDER_NAME + "\\\"",
          "build_path=" + CEF_ARTIFACTS_BUILD_PATH,
          "target=" + MODULE_TARGET,
          "platform=" + platform,
          "arch=" + ARCHI,
          "cpu_parallelism=" + use_openmp)

###############################################################################
###
### Compile Godot CEF module named GDCef and its subprocess
###
###############################################################################
def compile_gdnative_cef(path):
    info("Compiling Godot CEF module " + path)
    os.chdir(path)
    if OSTYPE == "Linux":
        gdnative_scons_cmd("x11")
    elif OSTYPE == "Darwin":
        gdnative_scons_cmd("macos")
    elif OSTYPE == "Windows" or OSTYPE == "MinGW":
        gdnative_scons_cmd("windows")
    else:
        fatal("Unknown archi " + OSTYPE + ": I dunno how to compile CEF module primary process")

###############################################################################
###
### Compile Godot CEF module named GDCef and its subprocess
###
###############################################################################
def create_gdextension_file():
    info("Creating Godot .gdextension file")
    with open(os.path.join(GDCEF_PATH, "gdcef.gdextension.in"), "r") as f:
        extension = f.read().replace("CEF_ARTIFACTS_FOLDER_NAME", CEF_ARTIFACTS_FOLDER_NAME)
    with open(os.path.join(CEF_ARTIFACTS_BUILD_PATH, "gdcef.gdextension"), "w") as f:
        f.write(extension)

###############################################################################
###
### Check if compilers are present (Windows)
###
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
                  "Install https://visualstudio.microsoft.com "
                  "and open an x64 Native Tools Command Prompt for VS 2022, with Administrator privilege")
        if os.path.isfile(binfile) == False:
            os.remove(cppfile)
            fatal("MS C++ compiler is not working. "
                  "Install https://visualstudio.microsoft.com "
                  "and open an x64 Native Tools Command Prompt for VS 2022, with Administrator privilege")
        if os.system(binfile) != 0:
            os.remove(cppfile)
            fatal("MS C++ compiler could not compile test program. "
                  "Install https://visualstudio.microsoft.com "
                  "and open an x64 Native Tools Command Prompt for VS 2022, with Administrator privilege")
        info("MS C++ Compiler OK")
        os.remove(cppfile)
        os.remove(binfile)
        os.remove(objfile)

###############################################################################
###
### Check for the minimal cmake version imposed by CEF
###
###############################################################################
def check_cmake_version():
    DOC_URL = "https://github.com/stigmee/doc-internal/blob/master/doc/install_latest_cmake.sh"
    info("Checking cmake version ...")
    if shutil.which("cmake") == None:
        fatal("It seems you have not CMake installed. For Linux see " + DOC_URL +
              " to update it before running this script. For Windows install "
              "the latest exe.")
    output = subprocess.check_output(["cmake", "--version"]).decode("utf-8")
    line = output.splitlines()[0]
    current_version = line.split()[2].split('-')[0]
    if version.parse(current_version) < version.parse(CMAKE_MIN_VERSION):
        fatal("Your CMake version is " + current_version + " but shall be >= "
              + CMAKE_MIN_VERSION + "\nSee " + DOC_URL + " to update it before "
              "running this script for Linux. For Windows install the latest exe.")

###############################################################################
###
### Check if build tools are present.
###
###############################################################################
def check_build_chain():
    info("Checking if the build chain is present")
    if not(shutil.which('cmake')):
        fatal("You need to install 'cmake' tool")
    if not(shutil.which('ninja') or shutil.which('make')):
        fatal("You need to install either 'ninja' or 'gnu makefile' tool")
    if (shutil.which('ninja') == None and OSTYPE == "Darwin"):
        fatal("You need to install 'ninja' tool")
    if type(SCONS) == str:
        if not(shutil.which(SCONS)):
            fatal("You need to install 'scons' tool")
    elif importlib.import_module("scons") == None:
        fatal("You need to install 'scons' tool")
    check_cmake_version()
    check_compiler()

###############################################################################
###
### Check if we run this script as Windows administrator. (experimental)
###
###############################################################################
def check_run_as_windows_administrator():
    if OSTYPE == "Windows":
        import ctypes
        if not ctypes.windll.shell32.IsUserAnAdmin():
            fatal("You shall run this script with administrator rights")

###############################################################################
###
### Since we have multiple demos and CEF artifacts are heavy (> 1 GB) we use
### aliases to fake Godot using a real local folder. This is an hack to save
### space on the hard disk. But for Windows users, this maybe problematic since
### Windows only allows aliases in admnistration mode.
###
###############################################################################
def prepare_godot_examples():
    info("Adding CEF artifacts for Godot demos:")
    for filename in os.listdir(GDCEF_EXAMPLES_PATH):
        path = os.path.join(GDCEF_EXAMPLES_PATH, filename)
        if os.path.isdir(path) and os.path.isfile(os.path.join(path, "project.godot")):
            info("  - Demo " + path)
            artifacts_path = os.path.join(path, CEF_ARTIFACTS_FOLDER_NAME)
            mkdir(os.path.dirname(artifacts_path))
            # For Windows user without admin rights: copy the created cef
            # artifacts folder in each demo folders.
            symlink(CEF_ARTIFACTS_BUILD_PATH, artifacts_path)

###############################################################################
###
### Final instructions for running GDCEF demos
###
###############################################################################
def final_instructions():
    print("")
    info("Compilation done with success!\n\n"
         "You can run your Godot editor " + GODOT_VERSION + " and try one of the demos located in '" + GDCEF_EXAMPLES_PATH + "'.\n"
         "All your CEF and Godot artifacts have been generated inside '" + CEF_ARTIFACTS_BUILD_PATH + "'.\n"
         "This folder can be used directly in your Godot project by copying it inside your project.\n"
         "Note: If you want use a different folder name, edit the value of CEF_ARTIFACTS_FOLDER_NAME in build.py and relaunch it.\n"
         "Note: in demos '" + CEF_ARTIFACTS_FOLDER_NAME + "' is not a folder but a pointer. We used a pointer since artifacts are heavy (+1GB) and we\n"
         "wanted to avoid you to loose space disk by copying the folder for each demo. For your application case, a folder is\nprobably what you want.\n")
    info("Have fun now! :)\n\n")

###############################################################################
###
### Clone GitHub repositories listed in a text file
###
###############################################################################
def clone_github_projects():
    repos_file = os.path.join(GDCEF_EXAMPLES_PATH, "repos.txt")
    info("Cloning GitHub real projects from " + repos_file)

    if not os.path.exists(repos_file):
        warning("The file " + repos_file + " does not exist. No real projects will be cloned.")
        return

    with open(repos_file, 'r') as f:
        for line in f:
            # Ignore empty lines and comments
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            # Extract the repo name from the URL
            match = re.search(r'github.com/[^/]+/([^/]+)', line)
            if not match:
                warning("Invalid GitHub URL: " + line)
                continue

            repo_name = match.group(1)
            # Remove extension if present (like .git)
            repo_name = repo_name.split('.')[0]
            repo_path = os.path.join(GDCEF_EXAMPLES_PATH, repo_name)

            # Check if the repo already exists
            if os.path.exists(repo_path):
                info("Repository " + repo_name + " already exists in " + repo_path)
                continue

            # Clone the repo
            try:
                info("Cloning " + line + " into " + repo_path)
                exec("git", "clone", "--recursive", line, repo_path)
            except Exception as e:
                warning("Error while cloning " + line + ": " + str(e))

###############################################################################
###
### Entry point
###
###############################################################################
if __name__ == "__main__":
    check_run_as_windows_administrator()
    check_paths()
    if GITHUB_GDCEF_RELEASE == None:
        check_build_chain()
        download_godot_cpp()
        compile_godot_cpp()
        download_cef()
        compile_cef()
        copy_cef_assets()
        create_version_file()
        compile_gdnative_cef(GDCEF_PATH)
        # MacOSX: use cefsimple.app instead of gdCefSubProcess
        if OSTYPE != "Darwin":
            compile_gdnative_cef(GDCEF_PROCESSES_PATH)
        create_gdextension_file()
    else:
        download_gdcef_release()
    clone_github_projects()
    prepare_godot_examples()
    final_instructions()
