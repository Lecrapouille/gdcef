# Chromium Embedded Framework as Godot native module

This document explains how is compiled for Window, the Godot native module (GDNative) for Chromium Embedded Framework (CEF).
This module is used by the Stigmee [project](https://github.com/stigmee/stigmee) and compiled automatically by
this [install](https://github.com/stigmee/install) for Linux while, for the moment, you will have to do some
manual steps for Windows.

* Tested successfully with Godot 3.4.2-stable, Linux and Windows environments. MacOS X compiles but does not work yet.
* Tested with the following CEF version : cef_binary_96.0.16+g89c902b+chromium-96.0.4664.55 that can be downloaded at
https://cef-builds.spotifycdn.com/index.html

## Environment

Example of installation :

```
<Project>\                                     <= Godot installation root (compile godot from here)
<Project>\godot-native\                        <= Base folder holding native modules
<Project>\godot-native\gdcef                   <= Code for the CEF module
<Project>\godot-native\gdcef_subprocess        <= Code of the sub-process CEF executable
<Project>\godot-native\godot-cpp               <= Godot C++ API and bindings (git cloned and to be compiled from here)
<Project>\godot-native\thirdparty\cef_binary   <= CEF distribution used to build the dependencies (downloaded  and to be compiled from here)
```

## Prerequisites

The following 2 prerequisite components *are NOT included* in this repository and must be added to the project before any compilation attempt:
- godot-native\godot-cpp
- godot-native\thirdparty\cef_binary

### godot-cpp: Godot C++ API

First of all, clone **recursively** the godot-cpp repository into `godot-native\godot-cpp` subfolder, using the appropriate branch (do not clone the master as you would end up with headers for the 4.0 version. Recursive cloning will include the appropriate godot-headers used to generate the C++ bindings.

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
```
cd godot-cpp
scons platform=windows target=release -j8
```
(use release_debug in rc distributions)
(use release in stable distributions)

### cef_binary: prebuilt Chromium Embedded Framework

Download the appropriate release (depending on your OS) from https://cef-builds.spotifycdn.com/index.html
and extract it into `godot-native/thirdparty/cef_binary`, then compile it like so.

```
cd ./thirdparty/cef_binary
mkdir build
cd build
cmake ..
cmake --build .
```

For Windows64, actual builds are using dynamic library, and default VS solutions is configured for static compilation. Therefore need to use VS to compile in Release mode, and you will need to change the compiler mode of the Release mode from `/MT` to `/MD`,
and add the 2 following preprocessor flags

* `_ITERATOR_DEBUG_LEVEL = 0;`                 under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.
* `_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH`       under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.


## Sub-Process executable compilation (./gdcef_subprocess)

This executable is needed in order for the CEF to spawn the various CEF sub-processes (gpu process, render handler...). The binary path is passed at runtime to the CEF Client.

```
📦gdcef_subprocess
 ┣ 📂src
 ┃ ┣ 📜gdcef_browser_app.cpp
 ┃ ┣ 📜gdcef_browser_app.h
 ┃ ┣ 📜gdcef_client.cpp
 ┃ ┣ 📜gdcef_client.h
 ┃ ┗ 📜main.cpp
 ┗ 📜SConstruct
```

To compile this source :

```
cd <Project>\godot-native\
mkdir build
cd .\gdcef_subprocess
scons target=release platform=windows -j8
```

The executable will be generated into the build directory. It should be placed into the appropriate godot project path (see Module configuration).

## Module compilation (./gdcef)

This directory contains the source of the gdcef library, allowing to generate the libgdcef.dll module. This dll file can then be loaded by the GDNative module (see Module configuration).

```
📦gdcef
 ┣ 📂src
 ┃ ┣ 📜gdcef.cpp
 ┃ ┣ 📜gdcef.h
 ┃ ┗ 📜gdlibrary.cpp
 ┗ 📜SConstruct
```

To compile this source :

```
cd <Project>\godot-native\
mkdir build
cd .\gdcef
scons target=release platform=windows -j8
```

## Module configuration

In order to configure the module :
- place the `libgdcef.dll` file and `gdcefSubProcess.exe` into the project root (main game directory)
- create the following 2 files under the `./build` directory

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

Ensure the lib is correctly loaded into your project (open the lib in the godot editor, make sur GDCef can be instanciated in GDScript).

## Exposed methods (as of 13/01/2022)

The following methods are part of the library at the moment, all in the GDCef class :

```
    void load_url(String url)
    void navigate_back()
    void navigate_forward()
    void do_message_loop_work()
    Ref<ImageTexture> get_texture()
    void reshape(int w, int h)
    void on_key_pressed(int key, bool pressed, bool up)
    void on_mouse_moved(int x, int y)
    void on_mouse_click(int button, bool mouse_up)
    void on_mouse_wheel(const int wDelta)
```

*IMPORTANT:* The following dependencies are not included in the repository and need to be copied into your godot project root (res://) along with the sub-process executable binary (they should be available in ./thirdparty/cef_binary), otherwise godot will complain about not being able to load the module dependancies at project startup. Those files are mandatory to correctly startup CEF :

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
