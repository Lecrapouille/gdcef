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
### This python script allows to compile CEF helloworld project for Linux or
### Windows.
###
###############################################################################

import os, sys, subprocess, hashlib, tarfile, shutil, glob, progressbar, urllib.request
from platform import machine, system
from pathlib import Path
from subprocess import run
from multiprocessing import cpu_count
from packaging import version
from shutil import move, copymode

###############################################################################
### Global user settings
# CEF version downloaded from https://cef-builds.spotifycdn.com/index.html
CEF_VERSION = "125.0.19+g3d8f1c9+chromium-125.0.6422.112"
CEF_TARGET = "Release"             # or "Debug"
MODULE_TARGET = "release"          # or "debug"
GODOT_CPP_TARGET = "template_release"       # or "template_debug"
GODOT_VERSION = "4.2"              # or "master" or "3.5" or "3.4"
CMAKE_MIN_VERSION = "3.19"         # Minimun CMake version needed for compiling CEF
# Scons is the build system used by Godot. For some people "scons" command does not
# work, they need to call "python -m SCons" command. The array is needed by the func
# calling scons.
SCONS = ["scons"]                  # or ["python", "-m", "SCons"]
# The name of the folder that will hold all CEF built artifacts.
CEF_BUILD_FOLDER_NAME = "cef_artifacts"
# When we are compiling demos we are creating a folder holding CEF build artifacts.
# But, in the aim to save space on your hard disk the folder is a pointer to folder
# CEF_ARTIFACTS_BUILD_PATH.
CEF_ARTIFACTS_FOLDER_NAME = "cef_artifacts"
# Use OpenMP for using CPU parallelim (i.e. for copying textures)
CEF_USE_CPU_PARALLELISM = "yes" # or "no"

###############################################################################
### Project internal paths local from this script. Do not change them!
PWD = os.getcwd()
GDCEF_PATH = os.path.join(PWD, "gdcef")
GDCEF_PROCESSES_PATH = os.path.join(PWD, "subprocess")
GDCEF_THIRDPARTY_PATH = os.path.join(PWD, "thirdparty")
THIRDPARTY_CEF_PATH = os.path.join(GDCEF_THIRDPARTY_PATH, "cef_binary")
THIRDPARTY_GODOT_PATH = os.path.join(GDCEF_THIRDPARTY_PATH, "godot-" + GODOT_VERSION)
GODOT_CPP_API_PATH = os.path.join(THIRDPARTY_GODOT_PATH, "cpp")
PATCHES_PATH = os.path.join(PWD, "patches")
GDCEF_EXAMPLES_PATH = os.path.join(PWD, "demos")
CEF_ARTIFACTS_BUILD_PATH = os.path.realpath(os.path.join("../../" + CEF_BUILD_FOLDER_NAME))

###############################################################################
### Type of operating system, AMD64, ARM64 ...
ARCHI = machine()
NPROC = str(cpu_count())
OSTYPE = system()

###############################################################################
### Green color message
def info(msg):
    print("\033[32m[INFO] " + msg + "\033[00m", flush=True)

###############################################################################
### Red color message + abort
def fatal(msg):
    print("\033[31m[FATAL] " + msg + "\033[00m", flush=True)
    sys.exit(2)

###############################################################################
### Equivalent to test -L e on alias + ln -s
def symlink(src, dst, force=False):
    p = Path(dst);
    if p.is_symlink():
        os.remove(p)
    elif force and p.is_file():
        os.remove(p)
    elif force and p.is_dir():
        rmdir(dst)
    os.symlink(src, dst)

###############################################################################
### Equivalent to cp --verbose
def copyfile(file_name, folder):
    dest = os.path.join(folder, os.path.basename(file_name))
    print("Copy " + file_name + " => " + dest)
    shutil.copyfile(file_name, dest)
    copymode(file_name, dest)

###############################################################################
### Equivalent to mkdir -p
def mkdir(path):
    Path(path).mkdir(parents=True, exist_ok=True)

###############################################################################
### Equivalent to rm -fr
def rmdir(top):
    if os.path.isdir(top):
        for root, dirs, files in os.walk(top, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        os.rmdir(top)

###############################################################################
### Equivalent to tar -xj
def untarbz2(tar_bz2_file_name, dest_dir):
    info("Unpacking " + tar_bz2_file_name + " ...")
    with tarfile.open(tar_bz2_file_name) as f:
        directories = []
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
### Search an expression (not a regexp) inside a file
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
### Needed for urllib.request.urlretrieve
### See https://stackoverflow.com/a/53643011/8877076
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
### Download artifacts
def download(url, destination):
    info("Download " + url + " into " + destination)
    urllib.request.urlretrieve(url, destination, reporthook=MyProgressBar())
    print('', flush=True)

###############################################################################
### Compute the SHA1 of the given artifact file
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
### Read a text file holding a SHA1 value
def read_sha1_file(path_sha1):
    file = open(path_sha1, "r")
    for line in file:
        return line # Just read 1st line
    return None

###############################################################################
### Give some path checks
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
### Download prebuild Chromium Embedded Framework if folder is not present
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
        fatal("Unknown archi " + OSTYPE + ": Cannot download Chromium Embedded Framework")

    # CEF already installed ? Installed with a different version ?
    # Compare our desired version with the one stored in the CEF README
    if grep(os.path.join(THIRDPARTY_CEF_PATH, "README.txt"), CEF_VERSION) != None:
        info(CEF_VERSION + " already downloaded")
    else:
        # Replace the '+' chars by URL percent encoding '%2B'
        CEF_URL_VERSION = CEF_VERSION.replace("+", "%2B")
        CEF_TARBALL = "cef_binary_" + CEF_URL_VERSION + "_" + CEF_ARCHI + ".tar.bz2"
        SHA1_CEF_TARBALL = CEF_TARBALL + ".sha1"
        info("Downloading Chromium Embedded Framework into " + THIRDPARTY_CEF_PATH + " ...")

        # Remove the CEF folder if exist and partial downloaded folder
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
### Patch Chromium Embedded Framework cefsimple example if not already made
def patch_cef():
    if os.path.isdir(THIRDPARTY_CEF_PATH):
        os.chdir(THIRDPARTY_CEF_PATH)
        info("Patching Chromium Embedded Framework")

        # Apply patches for Windows
        if OSTYPE == "Windows":
            shutil.copyfile(os.path.join(PATCHES_PATH, "CEF", "win", "libcef_dll_wrapper_cmake"),
                            "CMakeLists.txt")

###############################################################################
### Compile Chromium Embedded Framework cefsimple example if not already made
def compile_cef():
    if os.path.isdir(THIRDPARTY_CEF_PATH):
        patch_cef()

        os.chdir(THIRDPARTY_CEF_PATH)
        info("Compiling Chromium Embedded Framework in " + CEF_TARGET +
             " mode (inside " + THIRDPARTY_CEF_PATH + ") ...")

        # Windows: force compiling CEF as static library.
        if OSTYPE == "Windows":
            run(["cmake", "-DCEF_RUNTIME_LIBRARY_FLAG=/MD", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, "."], check=True)
            run(["cmake", "--build", ".", "--config", CEF_TARGET], check=True)
        else:
           mkdir("build")
           os.chdir("build")
           # Compile CEF if Ninja is available else use default GNU Makefile
           if shutil.which('ninja') != None:
               run(["cmake", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, ".."], check=True)
               run(["ninja", "-v", "-j" + NPROC, "cefsimple"], check=True)
           else:
               run(["cmake", "-G", "Unix Makefiles", "-DCMAKE_BUILD_TYPE=" + CEF_TARGET, ".."], check=True)
               run(["make", "cefsimple", "-j" + NPROC], check=True)

###############################################################################
### Copy Chromium Embedded Framework assets to your application build folder
def install_cef_assets():
    build_path = CEF_ARTIFACTS_BUILD_PATH
    mkdir(build_path)

    ### Get all CEF compiled artifacts needed for your application
    info("Installing Chromium Embedded Framework to " + build_path + " ...")
    locales = os.path.join(build_path, "locales")
    mkdir(locales)
    if OSTYPE == "Linux" or OSTYPE == "Darwin":
        # cp THIRDPARTY_CEF_PATH/build/tests/cefsimple/*.pak *.dat *.so locales/* build_path
        S = os.path.join(THIRDPARTY_CEF_PATH, "build", "tests", "cefsimple", CEF_TARGET)
        copyfile(os.path.join(S, "v8_context_snapshot.bin"), build_path)
        copyfile(os.path.join(S, "icudtl.dat"), build_path)
        for f in glob.glob(os.path.join(S, "*.pak")):
            copyfile(f, build_path)
        for f in glob.glob(os.path.join(S, "locales/*")):
            copyfile(f, locales)
        for f in glob.glob(os.path.join(S, "*.so")):
            copyfile(f, build_path)
        for f in glob.glob(os.path.join(S, "*.so.*")):
            copyfile(f, build_path)
    elif OSTYPE == "Windows":
        # cp THIRDPARTY_CEF_PATH/Release/*.bin THIRDPARTY_CEF_PATH/Release/*.dll build_path
        S = os.path.join(THIRDPARTY_CEF_PATH, CEF_TARGET)
        copyfile(os.path.join(S, "v8_context_snapshot.bin"), build_path)
        for f in glob.glob(os.path.join(S, "*.dll")):
            copyfile(f, build_path)
        # cp THIRDPARTY_CEF_PATH/Resources/*.pak *.dat locales/* build_path
        S = os.path.join(THIRDPARTY_CEF_PATH, "Resources")
        copyfile(os.path.join(S, "icudtl.dat"), build_path)
        for f in glob.glob(os.path.join(S, "*.pak")):
            copyfile(f, build_path)
        for f in glob.glob(os.path.join(S, "locales/*")):
            copyfile(f, locales)
    elif OSTYPE == "Darwin":
        # For Mac OS X rename cef_sandbox.a to libcef_sandbox.a since Scons search
        # library names starting by lib*
        os.chdir(os.path.join(THIRDPARTY_CEF_PATH, CEF_TARGET))
        shutil.copyfile("cef_sandbox.a", "libcef_sandbox.a")
        S = os.path.join(THIRDPARTY_CEF_PATH, CEF_TARGET, "Chromium Embedded Framework.framework")
        for f in glob.glob(S + "/Libraries*.dylib"):
            copyfile(f, build_path)
        for f in glob.glob(S + "/Resources/*"):
            copyfile(f, build_path)
    else:
        fatal("Unknown architecture " + OSTYPE + ": I dunno how to extract CEF artifacts")

###############################################################################
### Download Godot cpp wrapper needed for our gdnative code: CEF ...
def download_godot_cpp():
    if not os.path.exists(GODOT_CPP_API_PATH):
        info("Clone cpp wrapper for Godot " + GODOT_VERSION + " into " + GODOT_CPP_API_PATH)
        mkdir(GODOT_CPP_API_PATH)
        run(["git", "clone", "--recursive", "-b", GODOT_VERSION,
             "https://github.com/godotengine/godot-cpp", GODOT_CPP_API_PATH])

###############################################################################
### Compile Godot cpp wrapper needed for our gdnative code: CEF ...
def compile_godot_cpp():
    lib = os.path.join(GODOT_CPP_API_PATH, "bin", "libgodot-cpp*" + GODOT_CPP_TARGET + "*")
    if not os.path.exists(lib):
        info("Compiling Godot C++ API (inside " + GODOT_CPP_API_PATH + ") ...")
        os.chdir(GODOT_CPP_API_PATH)
        if OSTYPE == "Linux":
            run(SCONS + ["platform=linux", "target=" + GODOT_CPP_TARGET, "use_static_cpp=no",
                 "--jobs=" + NPROC], check=True)
        elif OSTYPE == "Darwin":
            run(SCONS + ["platform=osx", "macos_arch=" + ARCHI,
                 "target=" + GODOT_CPP_TARGET, "use_static_cpp=no",
                 "--jobs=" + NPROC], check=True)
        elif OSTYPE == "MinGW":
            run(SCONS + ["platform=windows", "use_mingw=True",
                 "target=" + GODOT_CPP_TARGET, "use_static_cpp=no",
                 "--jobs=" + NPROC], check=True)
        elif OSTYPE == "Windows":
            run(SCONS + ["platform=windows", "target=" + GODOT_CPP_TARGET, "use_static_cpp=no",
                 "--jobs=" + NPROC], check=True)
        else:
            fatal("Unknown architecture " + OSTYPE + ": I dunno how to compile Godot-cpp")

###############################################################################
### Common Scons command for compiling our Godot gdnative modules
def gdnative_scons_cmd(plateform):
    if GODOT_CPP_API_PATH == '':
        fatal('Please download and compile https://github.com/godotengine/godot-cpp and set GODOT_CPP_API_PATH')
    if OSTYPE == "Darwin":
        run(SCONS + ["api_path=" + GODOT_CPP_API_PATH,
             "cef_artifacts_folder=\\\"" + CEF_ARTIFACTS_FOLDER_NAME + "\\\"",
             "build_path=" + CEF_ARTIFACTS_BUILD_PATH,
             "target=" + MODULE_TARGET, "--jobs=" + NPROC,
             "arch=" + ARCHI, "platform=" + plateform,
             "cpu_parallelism=" + CEF_USE_CPU_PARALLELISM], check=True)
    else:
        run(SCONS + ["api_path=" + GODOT_CPP_API_PATH,
             "cef_artifacts_folder=\\\"" + CEF_ARTIFACTS_FOLDER_NAME + "\\\"",
             "build_path=" + CEF_ARTIFACTS_BUILD_PATH,
             "target=" + MODULE_TARGET, "--jobs=" + NPROC,
             "platform=" + plateform,
             "cpu_parallelism=" + CEF_USE_CPU_PARALLELISM], check=True)

###############################################################################
### Compile Godot CEF module named GDCef and its subprocess
def compile_gdnative_cef(path):
    info("Compiling Godot CEF module " + path)
    os.chdir(path)
    if OSTYPE == "Linux":
        gdnative_scons_cmd("x11")
    elif OSTYPE == "Darwin":
        gdnative_scons_cmd("osx")
    elif OSTYPE == "Windows": # or OSTYPE == "MinGW":
        gdnative_scons_cmd("windows")
    else:
        fatal("Unknown archi " + OSTYPE + ": I dunno how to compile CEF module primary process")

###############################################################################
### Compile Godot CEF module named GDCef and its subprocess
def create_gdextension_file():
    info("Creating Godot .gdextension file")
    with open(os.path.join(GDCEF_PATH, "gdcef.gdextension.in"), "r") as f:
        extension = f.read().replace("CEF_ARTIFACTS_FOLDER_NAME", CEF_ARTIFACTS_FOLDER_NAME)
    with open(os.path.join(CEF_ARTIFACTS_BUILD_PATH, "gdcef.gdextension"), "w") as f:
        f.write(extension)

###############################################################################
### Check if compilers are present (Windows)
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
### Check for the minimal cmake version imposed by CEF
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
### Since we have multiple examples and CEF artifacts are heavy we make alias
### on the build folder. On a real example you do not have to do it: simply
### install the build/ folder inside your Godot application.
def prepare_godot_examples():
    info("Adding CEF artifacts for Godot demos:")
    for filename in os.listdir(GDCEF_EXAMPLES_PATH):
        path = os.path.join(GDCEF_EXAMPLES_PATH, filename)
        if os.path.isdir(path) and os.path.isfile(os.path.join(path, "project.godot")):
            info("  - Demo " + path)
            artifacts_path = os.path.join(path, CEF_ARTIFACTS_FOLDER_NAME)
            mkdir(os.path.dirname(artifacts_path))
            symlink(CEF_ARTIFACTS_BUILD_PATH, artifacts_path)

###############################################################################
### Final instructions to run Godot demos
def final_instructions():
    rename_instruction = ""
    if CEF_BUILD_FOLDER_NAME != CEF_ARTIFACTS_FOLDER_NAME:
        rename_instruction = " and renamed as '" + CEF_ARTIFACTS_FOLDER_NAME + "'"

    print("")
    info("Compilation done with success!\n\n"
         "You can run your Godot editor " + GODOT_VERSION + " and try one of the demos located in '" + GDCEF_EXAMPLES_PATH + "'.\n"
         "All your CEF and Godot artifacts have been generated inside '" + CEF_ARTIFACTS_BUILD_PATH + "'.\n"
         "This folder can be used directly in your Godot project by copying it inside your project" + rename_instruction + ".\n"
         "Note: If you want use a different folder name, edit the value of CEF_ARTIFACTS_FOLDER_NAME in build.py and relaunch it.\n"
         "Note: in demos '" + CEF_ARTIFACTS_FOLDER_NAME + "' is not a folder but a pointer. We used a pointer since artifacts are heavy (+1GB) and we\n"
         "wanted to avoid you to loose space disk by copying the folder for each demo. For your application case, a folder is\nprobably what you want.\n")
    info("Have fun now! :)\n\n")

###############################################################################
### Entry point
if __name__ == "__main__":
    check_paths()
    check_cmake_version()
    check_compiler()
    download_godot_cpp()
    compile_godot_cpp()
    download_cef()
    compile_cef()
    install_cef_assets()
    compile_gdnative_cef(GDCEF_PATH)
    compile_gdnative_cef(GDCEF_PROCESSES_PATH)
    create_gdextension_file()
    prepare_godot_examples()
    final_instructions()
