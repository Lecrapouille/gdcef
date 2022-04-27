# Chromium Embedded Framework as Godot native module

This repository contains the Godot native module (GDNative) for [Chromium
Embedded Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) (CEF).
The module contains the code source of C++ wrapper classes on CEF API to be used
from Godot scripts and is compiled automatically by the
[install](https://github.com/stigmee/install) script `build.py` for Linux and
for Windows 10 (not yet for Mac OS).

This document explains how this module is compiled to make you understand global
ideas behind but for details, you will have to see directly inside the code
source. The detail design is described in this [document](doc/detailsdesign.md).

## Environment

Example of installation :

```
📦<Project>
 ┣ 📂godot-cpp                 <= Godot C++ API and bindings (git cloned and to be compiled from here)
 ┗ 📂godot-native              <= Base folder holding native modules
   ┗ 📂browser                 <= Base folder holding native CEF module
     ┣ 📂gdcef                 <= Code for the CEF module
     ┣ 📂gdcef_subprocess      <= Code of the sub-process CEF executable
     ┗ 📂cef_binary            <= CEF distribution used to build the dependencies (downloaded  and to be compiled from here)
```

The Stigmee tree project is a little more complex than depicted in this diagram:
it contains the Godot editor code have more native modules (Stigmark, IPFS ...)
but only modules concerning CEF are depicted in this document.

### godot-cpp: Godot C++ API

This first component must be present before doing any compilation attempt of
Godot modules (Stigmark, IPFS ...).

It is created when the Stigmee workspace is created thanks the
[manifest](https://github.com/stigmee/manifest) file of the tool `tsrc` or
`git-repo`.  What this repo tool does is to clone **recursively** the godot-cpp
repository, using the appropriate branch (do not clone the master as you would
end up with headers for the 4.0 version). Recursive cloning will include the
appropriate godot-headers used to generate the C++ bindings. `godot-cpp`
contains the Godot API like if we were directly inside the code source of the
Godot editor.

```
git clone --recursive -b 3.4 https://github.com/godotengine/godot-cpp
Cloning into 'godot-cpp'...
remote: Enumerating objects: 4763, done.
remote: Counting objects: 100% (955/955), done.
remote: Compressing objects: 100% (417/417), done.
remote: Total 4763 (delta 562), reused 598 (delta 531), pack-reused 3808
Receiving objects: 100% (4763/4763), 3.56 MiB | 15.24 MiB/s, done.
Resolving deltas: 100% (3083/3083), done.
Submodule 'godot-headers' (https://github.com/godotengine/godot-headers) registered for path 'godot-headers'
Cloning into '<Project>\godot-native\godot-cpp/godot-headers'...
remote: Enumerating objects: 801, done.
remote: Counting objects: 100% (144/144), done.
remote: Compressing objects: 100% (111/111), done.
remote: Total 801 (delta 65), reused 74 (delta 23), pack-reused 657
Receiving objects: 100% (801/801), 1.99 MiB | 11.24 MiB/s, done.
Resolving deltas: 100% (498/498), done.
Submodule path 'godot-headers': checked out 'd1596b939d6c9f5df86655ea617713ef321ad938'
```

The `build.py`, for compiling `godot-cpp`, will call a command similar to these
lines:

```
cd godot-cpp
scons platform=windows target=release -j8
```

### cef_binary: prebuilt Chromium Embedded Framework

The second component is not created when the Stigmee workspace is created by the
repo tool but, instead, shall be downloaded from
https://cef-builds.spotifycdn.com/index.html which is done automatically by the
[install](https://github.com/stigmee/install) script `build.py`. The folder
contains the CEF prebuilt. Our build script knows your operating system and the
desired CEF version: a check inside the CEF's README (if present) allows it to
know if CEF has been downloaded and if the version is the good one. If the
version mismatch the old CEF version is removed and the new one is downlaoded.

The `build.py`, to compile CEF, will call a command similar to these lines:

```
cd ./thirdparty/cef_binary
mkdir build
cd build
cmake ..
make --build .
```

For Windows64, actual builds are using dynamic library, and default VS solutions
is configured for static compilation. Therefore need to use VS to compile in
Release mode, and you (the build script) will change the compiler mode of the
Release mode from `/MT` to `/MD`, and add the 2 following preprocessor flags:

* `_ITERATOR_DEBUG_LEVEL = 0;`                 under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.
* `_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH`       under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.

For Linux it seems not possible to compile in static, as consequence the
`libcef.so` is quite fat: more than 1 gigabytes which is a factor more than the
one for Windows (probably because this last knows better that Linux which symbol
to export). The reason of dynamic library instead of static is given
[here](https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11182) which is not
important in our case because Stigmee is under GPL licence.

*IMPORTANT:* The following dependencies are not included in the repository and
need to be copied into your Godot project root (res://) along with the
sub-process executable binary (they should be available in
./thirdparty/cef_binary), otherwise Godot will complain about not being able to
load the module dependencies at project startup. The destination folder is
https://github.com/stigmee/stigmee inside its `build` folder (to be
created). Those files are mandatory to correctly startup CEF:

```
chrome_elf.dll       <- thirdparty\cef_binary\Release
d3dcompiler_47.dll   <- thirdparty\cef_binary\Release
icudtl.dat           <- thirdparty\cef_binary\Release
libEGL.dll           <- thirdparty\cef_binary\Release
libGLESv2.dll        <- thirdparty\cef_binary\Release
libcef.dll           <- thirdparty\cef_binary\Release
snapshot_blob.bin    <- thirdparty\cef_binary\Release
v8_context_snapshot.bin   <- thirdparty\cef_binary\Release
chrome_100_percent.pak    <- from thirdparty\cef_binary\Resources
chrome_200_percent.pak    <- from thirdparty\cef_binary\Resources
icudtl.dat      <- from thirdparty\cef_binary\Resources
resources.pak   <- from thirdparty\cef_binary\Resources
```

Again, the `build.py` will do it for you.

### Sub-Process executable compilation (./gdcef_subprocess)

This executable is needed in order for the CEF to spawn the various CEF
sub-processes (GPU process, render handler...). The binary path is passed at
runtime to the CEF Client.

```
📦gdcef_subprocess
 ┣ 📂src
 ┃ ┣ 📜gdcef_browser_app.cpp
 ┃ ┣ 📜gdcef_browser_app.hpp
 ┃ ┣ 📜gdcef_client.cpp
 ┃ ┣ 📜gdcef_client.hpp
 ┃ ┗ 📜main.cpp
 ┗ 📜SConstruct
```

To compile this source :

```
cd <Project>\godot-native\
mkdir build
cd .\gdcef_subprocess
scons target=release platform=windows workspace=$WORKSPACE_STIGMEE godot_version=3.4.3 -j8
```

The executable will be created as `gdcefSubProcess.exe`. It should be placed
into the appropriate our godot project https://github.com/stigmee/stigmee inside
its `build` folder (to be created). Again the `build.py` will do it for you.

### Module compilation (./gdcef)

This directory contains the source of the gdcef library, allowing to generate
the `libgdcef.dll` module. This dll file can then be loaded by the GDNative
module (see Module configuration).

```
📦gdcef
 ┣ 📂src
 ┃ ┣ 📜gdcef.cpp
 ┃ ┣ 📜gdcef.hpp
 ┃ ┣ 📜gdbrowser.cpp
 ┃ ┣ 📜gdbrowser.hpp
 ┃ ┣ 📜gdlibrary.cpp
 ┃ ┗ 📜...
 ┗ 📜SConstruct
```

To compile this source :

```
cd <Project>\godot-native\
mkdir build
cd .\gdcef
scons target=release platform=windows -j8 workspace=$WORKSPACE_STIGMEE godot_version=3.4.3 -j8
```

The executable will be generated into the build directory. It should be placed
into the appropriate our godot project https://github.com/stigmee/stigmee inside
its `build` folder (to be created). Again the `build.py` will do it for you.

## Module configuration

In order to be used by Godot, create the following 2 files under the `./build`
directory:

- gdcef.gdns:
```
[gd_resource type="NativeScript" load_steps=2 format=2]

[ext_resource path="res://build/gdcef.gdnlib" type="GDNativeLibrary" id=1]

[resource]
resource_name = "gdcef"
class_name = "GDCef"
library = ExtResource( 1 )
script_class_name = "GDCef"
```

- gdcef.gdnlib:
```
[general]

singleton=false
load_once=true
symbol_prefix="godot_"
reloadable=false

[entry]

OSX.64="res://build/libgdcef.dylib"
Windows.64="res://build/libgdcef.dll"
X11.64="res://build/libgdcef.so"

[dependencies]

OSX.64=[ "res://build/libcef.dylib" ]
Windows.64=[ "res://build/libcef.dll" ]
X11.64=[ "res://build/libcef.so" ]
```

Ensure the lib is correctly loaded into your project (open the lib in the Godot
editor, make sure GDCef can be instantiated in GDScript).
