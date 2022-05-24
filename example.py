#!/usr/bin/env python3
###############################################################################
## Stigmee: The art to sanctuarize knowledge exchanges.
## Copyright 2021-2022 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of Stigmee.
##
## Stigmee is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################
###
### This python script allows to compile CEF helloworld project for Linux or
### Windows.
###
###############################################################################

import os, sys, wget, subprocess, hashlib, tarfile, wget, shutil, glob
from platform import machine, system
from pathlib import Path
from subprocess import run
from multiprocessing import cpu_count

###############################################################################
### Global user settings
CEF_VERSION = "100.0.24+g0783cf8+chromium-100.0.4896.127"
CEF_TARGET = "Release"     # "Debug"
MODULE_TARGET = "release"  # "debug"

PWD = os.getcwd()
GDCEF_PATH = os.path.join(PWD, "gdcef")
GDCEF_PROCESSES_PATH = os.path.join(PWD, "gdcef_subprocess")
GDCEF_THIRDPARTY_PATH = os.path.join(PWD, "thirdparty")
CEF_PATH = os.path.join(GDCEF_THIRDPARTY_PATH, "cef_binary")
GDCEF_EXAMPLE_PATH = os.path.join(PWD, "example")
GDCEF_EXAMPLE_BUILD_PATH = os.path.join(GDCEF_EXAMPLE_PATH, "build")
GODOT_CPP_API_PATH=''

###############################################################################
### Type of operating system, AMD64, ARM64 ...
ARCHI = machine()
NPROC = str(cpu_count())
OSTYPE = system()
if os.name == "nt" and get_platform().startswith("mingw"):
    OSTYPE = "MinGW"

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
### Equivalent to cp --verbose
def copyfile(file_name, folder):
    dest = os.path.join(folder, os.path.basename(file_name))
    print("Copy " + file_name + " => " + dest)
    shutil.copyfile(file_name, dest)

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
### Download artifacts
def download(url):
    wget.download(url)
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
    for path in [PWD, GDCEF_PATH, GDCEF_PROCESSES_PATH, GDCEF_EXAMPLE_PATH,
                 GODOT_CPP_API_PATH]:
        if not os.path.isdir(path):
            fatal('Folder ' + path + ' does not exist!')

        # Remove the example build folder to avoid messed up with Stigmee build
        # using alias
        p = Path(GDCEF_EXAMPLE_BUILD_PATH);
        if p.is_symlink():
            os.remove(GDCEF_EXAMPLE_BUILD_PATH)
        elif p.is_dir():
            rmdir(GDCEF_EXAMPLE_BUILD_PATH)
        elif p.exists():
            fatal('Please remove manually ' + GDCEF_EXAMPLE_BUILD_PATH + ' and recall this script')

###############################################################################
### Download prebuild Chromium Embedded Framework if folder is not present
def download_cef():
    if OSTYPE == "Linux":
        if ARCHI == "x86_64":
            CEF_ARCHI = "linux64"
        else:
            CEF_ARCHI = "linuxarm"
    elif OSTYPE == "darwin":
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
    if grep(os.path.join(CEF_PATH, "README.txt"), CEF_VERSION) != None:
        info(CEF_VERSION + " already downloaded")
    else:
        # Replace the '+' chars by URL percent encoding '%2B'
        CEF_URL_VERSION = CEF_VERSION.replace("+", "%2B")
        CEF_TARBALL = "cef_binary_" + CEF_URL_VERSION + "_" + CEF_ARCHI + ".tar.bz2"
        info("Downloading Chromium Embedded Framework into " + CEF_PATH + " ...")

        # Remove the CEF folder if exist and partial downloaded folder
        mkdir(GDCEF_THIRDPARTY_PATH)
        os.chdir(GDCEF_THIRDPARTY_PATH)
        rmdir("cef_binary")

        # Download CEF at https://cef-builds.spotifycdn.com/index.html
        URL = "https://cef-builds.spotifycdn.com/" + CEF_TARBALL
        info(URL)
        download(URL)
        download(URL + ".sha1")
        if compute_sha1(CEF_TARBALL) != read_sha1_file(CEF_TARBALL + ".sha1"):
            os.remove(CEF_TARBALL)
            os.remove(CEF_TARBALL + ".sha1")
            fatal("Downloaded CEF tarball does not match expected SHA1. Please retry!")

        # Simplify the folder name by removing the complex version number
        untarbz2(CEF_TARBALL, CEF_PATH)

        # Remove useless files
        os.remove(CEF_TARBALL)
        os.remove(CEF_TARBALL + ".sha1")

###############################################################################
### Compile Chromium Embedded Framework cefsimple example if not already made
def compile_cef():
    if os.path.isdir(CEF_PATH):
        os.chdir(CEF_PATH)
        info("Compiling Chromium Embedded Framework in " + CEF_TARGET +
             " mode (inside " + CEF_PATH + ") ...")

        # Apply patches for Windows
        if OSTYPE == "Windows":
            shutil.copyfile(os.path.join(STIGMEE_INSTALL_PATH, "patch", "CEF", "win", "libcef_dll_wrapper_cmake"),
                            "CMakeLists.txt")

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
### Copy Chromium Embedded Framework assets to Stigmee build folder
def install_cef_assets():
    build_path = GDCEF_EXAMPLE_BUILD_PATH
    mkdir(build_path)

    ### Get all CEF compiled artifacts needed for Stigmee
    info("Installing Chromium Embedded Framework to " + build_path + " ...")
    locales = os.path.join(build_path, "locales")
    mkdir(locales)
    if OSTYPE == "Linux" or OSTYPE == "Darwin":
        # cp CEF_PATH/build/tests/cefsimple/*.pak *.dat *.so locales/* build_path
        S = os.path.join(CEF_PATH, "build", "tests", "cefsimple", CEF_TARGET)
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
        # cp CEF_PATH/Release/*.bin CEF_PATH/Release/*.dll build_path
        S = os.path.join(CEF_PATH, CEF_TARGET)
        copyfile(os.path.join(S, "v8_context_snapshot.bin"), build_path)
        for f in glob.glob(os.path.join(S, "*.dll")):
            copyfile(f, build_path)
        # cp CEF_PATH/Resources/*.pak *.dat locales/* build_path
        S = os.path.join(CEF_PATH, "Resources")
        copyfile(os.path.join(S, "icudtl.dat"), build_path)
        for f in glob.glob(os.path.join(S, "*.pak")):
            copyfile(f, build_path)
        for f in glob.glob(os.path.join(S, "locales/*")):
            copyfile(f, locales)
    elif OSTYPE == "Darwin":
        # For Mac OS X rename cef_sandbox.a to libcef_sandbox.a since Scons search
        # library names starting by lib*
        os.chdir(os.path.join(CEF_PATH, CEF_TARGET))
        shutil.copyfile("cef_sandbox.a", "libcef_sandbox.a")
        S = os.path.join(CEF_PATH, CEF_TARGET, "Chromium Embedded Framework.framework")
        for f in glob.glob(S + "/Libraries*.dylib"):
            copyfile(f, build_path)
        for f in glob.glob(S + "/Resources/*"):
            copyfile(f, build_path)
    else:
        fatal("Unknown architecture " + OSTYPE + ": I dunno how to extract CEF artifacts")

###############################################################################
### Common Scons command for compiling our Godot gdnative modules
def gdnative_scons_cmd(plateform):
    if GODOT_CPP_API_PATH == '':
        fatal('Please download and compile https://github.com/godotengine/godot-cpp and set GODOT_CPP_API_PATH')
    if OSTYPE == "Darwin":
        run(["scons", "api_path=" + GODOT_CPP_API_PATH,
             "build_path=" + GDCEF_EXAMPLE_BUILD_PATH,
             "target=" + MODULE_TARGET, "--jobs=" + NPROC,
             "arch=" + ARCHI, "platform=" + plateform], check=True)
    else: # FIXME "arch=" + ARCHI not working
        run(["scons", "api_path=" + GODOT_CPP_API_PATH,
             "build_path=" + GDCEF_EXAMPLE_BUILD_PATH,
             "target=" + MODULE_TARGET, "--jobs=" + NPROC,
             "platform=" + plateform], check=True)

###############################################################################
### Compile Godot CEF module named GDCef and its subprocess
def compile_gdnative_cef(path):
    info("Compiling Godot CEF module " + path)
    os.chdir(path)
    if OSTYPE == "Linux":
        gdnative_scons_cmd("x11")
    elif OSTYPE == "Darwin":
        gdnative_scons_cmd("osx")
    elif OSTYPE == "Windows" or OSTYPE == "MinGW":
        gdnative_scons_cmd("windows")
    else:
        fatal("Unknown archi " + OSTYPE + ": I dunno how to compile CEF module primary process")

###############################################################################
### Entry point
if __name__ == "__main__":
    argc = len(sys.argv)
    if argc != 2:
        print('Command line: ' + sys.argv[0] + ' <path to Godot C++ API>')
        print('Where:\n  <path to Godot C++ API> means the path to a compiled https://github.com/godotengine/godot-cpp')
        sys.exit(-1)

    GODOT_CPP_API_PATH = sys.argv[1]

    check_paths()
    download_cef()
    install_cef_assets()
    compile_gdnative_cef(GDCEF_PATH)
    compile_gdnative_cef(GDCEF_PROCESSES_PATH)
