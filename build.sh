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

# Green color message
function msg
{
    echo -e "\033[32m*** $*\033[00m"
}

# Red color message
function err
{
    echo -e "\033[31m*** $*\033[00m"
}

### Instal system packages needed for compiling Godot
function install_prerequisite
{
    msg "Installing prerequesite packages on your system ..."

    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        sudo apt-get install build-essential scons pkg-config libx11-dev libxcursor-dev libxinerama-dev \
             libgl1-mesa-dev libglu-dev libasound2-dev libpulse-dev libudev-dev libxi-dev libxrandr-dev yasm
    elif [[ "$OSTYPE" == "freebsd"* ]]; then
        sudo pkg install py37-scons pkgconf xorg-libraries libXcursor libXrandr libXi xorgproto libGLU \
             alsa-lib pulseaudio yasm
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        brew install scons yasm cmake
    else
        err "Unknown architecture. Install needed packages for Stigmee by yourself"
        return
    fi
}

### Clone godot-cpp and compile it
function install_godotcpp
{
    # Git clone godot-cpp if the folder is not present
    if [ ! -d godot-cpp ]; then
        msg "Downloading Godot-cpp ..."
        git clone https://github.com/godotengine/godot-cpp -b 3.4 --recurse
    fi

    # Compile godot-cpp if not already made
    (cd godot-cpp
     if [ ! -f bin/libgodot-cpp* ]; then
         msg "Compiling Godot-cpp ..."
         if [[ "$OSTYPE" == "linux-gnu"* ]]; then
             scons platform=linux target=release -j$(nproc)
         elif [[ "$OSTYPE" == "freebsd"* ]]; then
             scons platform=linux target=release -j$(nproc)
         elif [[ "$OSTYPE" == "darwin"* ]]; then
             ARCHI=`uname -m`
             if [[ "$ARCHI" == "x86_64" ]]; then
                 scons platform=osx macos_arch=x86_64 --jobs=$(sysctl -n hw.logicalcpu)
             else
                 scons platform=osx macos_arch=arm64 --jobs=$(sysctl -n hw.logicalcpu)
             fi
         else
             scons platform=windows target=release -j$(nproc)
         fi
     fi
    )
}

### Compile all Godot modules needed for Stigmee project
function install_stigmee_modules
{
    msg "Installing Godot modules needed for Stigmee ..."
    (cd gdcef_primary && ./build.sh)
    (cd gdcef_secondary && ./build.sh)
}

### Script entry point
mkdir -p build
install_prerequisite
install_godotcpp
install_stigmee_modules
