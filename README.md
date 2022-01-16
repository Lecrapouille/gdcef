# gdcef (GDNative module)

GDNative module for CEF integration into Godot, including a demo project

* Tested successfully with Godot 3.4.2-stable
* Along with the following CEF version : cef_binary_96.0.16+g89c902b+chromium-96.0.4664.55_windows64.tar.bz2

## Environment

Example of installation :

```
<Godot_Home>\                                     <= godot installation root (compile godot from here)
<Godot_Home>\godot-native\                        <= base of all native modules
<Godot_Home>\godot-native\gdcef                   <= Main code of the module
<Godot_Home>\godot-native\gdcef_subprocess        <= Code of the sub-process executable
<Godot_Home>\godot-native\godot-cpp               <= godot c++ bindings clone (compile the c++ bindings from here)
<Godot_Home>\godot-native\thirdparty\cef_binary   <= CEF distribution used to build the dependencies
```

## Prerequisites

the following 2 prerequisite components are NOT included in this repository and must be added to the project before any compilation attempt

### ./godot-cpp

First of all, clone the godot-cpp repository into 'godot-cpp' subfolder, using the appropriate branch (do not clone the master as you would end up with headers for the 4.0 version.
Recursive cloning will also include the appropriate godot-headers used to generate the c++ bindings.

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
Cloning into '<Godot_Home>\godot-native\godot-cpp/godot-headers'...
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

### ./thirdparty/cef_binary

Download the appropriate release (depending on your OS) from here :
https://cef-builds.spotifycdn.com/index.html#windows64
And extract it into ./thirdparty/cef_binary, then compile it like so.

```
cd ./thirdparty/cef_binary
mkdir build
cd build
cmake ..
cmake --build .
```

Actual builds are using dynamic library, and default VS solutions is configured for static compilation. Therefore need to use VS to compile in Release mode, and you will need to change the compiler mode of the Release mode from /MT to /MD,
and add the 2 following preprocessor flags

* _ITERATOR_DEBUG_LEVEL = 0;                 under C/C++ >> Preprocessor >> PreprocessorDefinitions. 
* _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH       under C/C++ >> Preprocessor >> PreprocessorDefinitions. 


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
cd <Godot_Home>\godot-native\
mkdir build
cd .\gdcef_subprocess
scons target=release platform=windows -j8
```

The executable will be generated into the build directory. It should be placed into the appropriate godot project path (see Module configuration) 


## Module compilation (./gdcef)

This directory contains the source of the gdcef library, allowing to generate the libgdcef.dll module.  This dll file can then be loaded by the GDNative module (see Module configuration)

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
cd <Godot_Home>\godot-native\
mkdir build
cd .\gdcef
scons target=release platform=windows -j8
```


## Module configuration

In order to configure the module :
- place the libgdcef.dll file and gdcefSubProcess.exe into the project root (main game directory)
- create the following 2 files under the ./lib directory

gdcef.gdns
```
[gd_resource type="NativeScript" load_steps=2 format=2]

[ext_resource path="res://lib/gdcef.gdnlib" type="GDNativeLibrary" id=1]

[resource]
resource_name = "gdcef"
class_name = "GDCef"
library = ExtResource( 1 )
script_class_name = "GDCef"
```

gdcef.gdnlib
```
[general]

singleton=false
load_once=true
symbol_prefix="godot_"
reloadable=false

[entry]

OSX.64="res://libgdcef.dylib"
Windows.64="res://libgdcef.dll"
X11.64="res://libgdcef.so"

[dependencies]

OSX.64=[  ]
Windows.64=[ "res://libcef.dll" ]
X11.64=[  ]
```

Ensure the lib is correctly loaded into your project (open the lib in the godot editor, make sur GDCef can be instanciated in GDScript)


## Exposed methods (as of 13/01/2022)

The following methods are part of the library at the moment, all in the GDCef class :

```
    load_url
    navigate_back
    navigate_forward
    do_message_loop_work
    get_texture
    reshape
    on_key_pressed
    on_mouse_moved
    on_mouse_click
    on_mouse_wheel
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
