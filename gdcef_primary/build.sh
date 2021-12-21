#!/bin/bash -e
###############################################################################
## Stigmee: A 3D browser and decentralized social network.
## Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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

### Green color message
function msg
{
    echo -e "\033[32m*** $*\033[00m"
}

### Red color message
function err
{
    echo -e "\033[31m*** $*\033[00m"
}

### Number of CPU cores
NPROC=1
if [[ "$OSTYPE" == "darwin"* ]]; then
    NPROC=`sysctl -n hw.logicalcpu`
else
    NPROC=`nproc`
fi

### Clone prebuild Chromium Embedded Framework and compile it
function install_cef
{
    # Download and decompress if folder is not present
    if [ ! -d ../thirdparty/cef_binary ]; then
        msg "Downloading Chromium Embedded Framework ..."
        UNAMEM=`uname -m`
        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            if [[ "$UNAMEM" == "x86_64" ]]; then
                ARCHI="linux64"
            else
                ARCHI="linuxarm"
            fi
        elif [[ "$OSTYPE" == "freebsd"* ]]; then
            if [[ "$UNAMEM" == "x86_64" ]]; then
                ARCHI="linux64"
            else
                ARCHI="linuxarm"
            fi
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            if [[ "$UNAMEM" == "x86_64" ]]; then
                ARCHI="macosx64"
            else
                ARCHI="macosarm64"
            fi
        else
            err "Unknown archi. Cannot download Chromium Embedded Framework"
            exit 1
        fi

        msg "Downloading Chromium Embedded Framework v96 for $ARCHI ..."
        WEBSITE=https://cef-builds.spotifycdn.com
        CEF_TARBALL=cef_binary_96.0.14%2Bg28ba5c8%2Bchromium-96.0.4664.55_$ARCHI.tar.bz2

        mkdir -p ../thirdparty
        (cd ../thirdparty
         wget -c $WEBSITE/$CEF_TARBALL -O - | tar -xj
         mv cef_binary* cef_binary
        )
    fi

    ### Compile Chromium Embedded Framework if not already made
    if [ ! -f ../thirdparty/cef_binary/build/tests/cefsimple/Release/cefsimple ]; then
        msg "Compiling Chromium Embedded Framework ..."
        (cd ../thirdparty/cef_binary
         mkdir -p build
         cd build
         cmake -DCMAKE_BUILD_TYPE=Release ..
         VERBOSE=1 make -j$NPROC cefsimple
        )
    fi
}

### Compile Godot CEF module named GDCef
function compile_primary_gdcef
{
    if [ ! -f libgdcef* ]; then
        msg "Compiling Godot CEF module (primary process) ..."

        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            scons platform=linux target=release -j$NPROC
        elif [[ "$OSTYPE" == "freebsd"* ]]; then
            scons platform=linux target=release -j$NPROC
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            ARCHI=`uname -m`
            if [[ "$ARCHI" == "x86_64" ]]; then
                scons platform=osx arch=x86_64 --jobs=$NPROC
            else
                scons platform=osx arch=arm64 --jobs=$NPROC
            fi
        else
            scons platform=windows target=release -j$NPROC
        fi
    fi
}

### Get all CEF compiled stuffs needed for Godot
function get_compiled_assets
{
    if [[ "$OSTYPE" == "darwin"* ]]; then
        D=../build
        S="../thirdparty/cef_binary/Release/Chromium Embedded Framework.framework/Libraries"
        cp -R "$S/"*.dylib $D

        S="../thirdparty/cef_binary/Release/Chromium Embedded Framework.framework/Resources"
        cp -R "$S/" $D
    else
        D=../build
        S=../thirdparty/cef_binary/build/tests/cefsimple/Release
        cp -r $S/*.pak $S/*.so* $S/*.dll $S/locales $S/v8_context_snapshot.bin $S/icudtl.dat $D
    fi
}

### Main
install_cef
compile_primary_gdcef
get_compiled_assets
