#!/bin/bash -e
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

### Debug or release ?
TARGET="$1"
if [ "$TARGET" == "debug" ]; then
   GODOT_TARGET=debug
   CEF_TARGET=Debug
elif [ "$TARGET" == "release" -o "$TARGET" == "" ]; then
   GODOT_TARGET=release
   CEF_TARGET=Release
elif [ "$TARGET" == "clean" ]; then
   rm -fr src/*.o
   exit 0
else
   err "Invalid target. Shall be clean or debug or release or shall be empty for release"
   exit 1
fi

### Number of CPU cores
NPROC=
if [[ "$OSTYPE" == "darwin"* ]]; then
    NPROC=`sysctl -n hw.logicalcpu`
else
    NPROC=`nproc`
fi

### Compile Godot CEF module named GDCef
function compile_secondary_gdcef
{
    if [ ! -f libgdcef* ]; then
        msg "Compiling Godot CEF_secondary module (secondary) ..."

        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            scons platform=linux target=$GODOT_TARGET --jobs=$NPROC
        elif [[ "$OSTYPE" == "freebsd"* ]]; then
            scons platform=linux target=$GODOT_TARGET --jobs=$NPROC
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            ARCHI=`uname -m`
            if [[ "$ARCHI" == "x86_64" ]]; then
                scons platform=osx arch=x86_64 target=$GODOT_TARGET --jobs=$NPROC
            else
                scons platform=osx arch=arm64 target=$GODOT_TARGET --jobs=$NPROC
            fi
        else
            scons platform=windows target=$GODOT_TARGET --jobs=$NPROC
        fi
    fi
}

### Main (depends on success obtained from ../gdcef_primary/build.sh)
compile_secondary_gdcef
msg "Cool! gdcef_secondary compiled with success"
