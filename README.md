# Chromium Embedded Framework as Godot 3.4 native module

This repository contains the Godot native module (GDNative) wrapping [Chromium
Embedded Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) (CEF),
that we have named `gdcef`. The code source of this module is made in C++ and
implements some classes wrapping a subset of the CEF API that can be directly
usable in Godot scripts (gdscript) but feel free to help us implemented other
features.

A minimal CEF example is given. It is automatically compiled by the install
`build.py` script. A concrete Godot application using CEF can be find
[here](https://github.com/stigmee/stigmee).

*IMPORTANT:* This current repository is a fork of [this original
repo](https://github.com/stigmee/gdnative-cef) (GPLv3) with a more permissive
license (MIT). Indeed, since the original project will no longer be developped
by their original authors, we, original authors, have accepted the fork of the
original repository under a new licence.

## CEF API

### GDCef

Class deriving from Godot's Node and interfacing Chromium Embedded Framework.
This class can create isntances of GDBrowserView and manage their lifetime.

| Godot function name | arguments                                       | return         | comment                                                                                                                                                                                                 |
|---------------------|-------------------------------------------------|----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| _process            | dt: float                                       | void           | Hidden function called automatically by Godot and call CEF internal pump messages.                                                                                                                      |
| create_browser      | url: string, name: string, width: int height: int | GDBrowserView* | Create a browser view and store its instance inside the internal. url: the page link. name: the browser name. width: the width dimension of the document. height: the height dimension of the document. |
| shutdown            |                                                 |                | Release CEF memory and sub CEF processes are notified that the application is exiting. All browsers are destroyed.                                                                                      |

### GDBrowserView

Class wrapping the CefBrowser class and export methods for Godot script.
This class is instanciate by GDCef.

| Godot function name   | arguments                                                   | return            | comment                                                                                                                                                                                                                                     |
|-----------------------|-------------------------------------------------------------|-------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| close                 |                                                             |                   | Close the browser.                                                                                                                                                                                                                          |
| id                    |                                                             | int               | Return the unique browser identifier.                                                                                                                                                                                                       |
| is_valid              |                                                             | bool              | Return True if this object is currently valid.                                                                                                                                                                                              |
| get_texture           |                                                             | Ref ImageTexture  | Return the Godot texture holding the page content to other Godot element that needs it for the rendering.                                                                                                                                   |
| use_texture_from      |                                                             |  Ref ImageTexture |  Return the Godot texture holding the page content to other Godot element that needs it for the rendering.                                                                                                                                  |
| set_zoom_level        | delta: float                                                |                   | Set the render zoom level.                                                                                                                                                                                                                  |
| load_url              | url: string                                                 |                   | Load the given web page                                                                                                                                                                                                                     |
| is_loaded             |                                                             | bool              | Return true if a document has been loaded in the browser.                                                                                                                                                                                   |
| get_url               |                                                             | string            | Get the current url of the browser.                                                                                                                                                                                                         |
| stop_loading          |                                                             |                   | Stop loading the page.                                                                                                                                                                                                                      |
| has_previous_page     |                                                             | bool              | Return true if the browser can navigate to the previous page.                                                                                                                                                                               |
| has_next_page         |                                                             | bool              | Return true if the browser can navigate to the next page.                                                                                                                                                                                   |
| previous_page         |                                                             |                   | Navigate to the previous page if possible.                                                                                                                                                                                                  |
| next_page             |                                                             |                   | Navigate to the next page if possible.                                                                                                                                                                                                      |
| resize                |  width: int, height: int                                    |                   | Reshape the windows size.                                                                                                                                                                                                                   |
| set_viewport          |  x: float, y: float, width: float, height: float            |                   | the rectangle on the surface where to display the web document. Values are in percent of the dimension on the surface. If this function is not called default values are: x = y = 0 and w = h = 1 meaning the whole surface will be mapped. |
| on_key_pressed        | key: int, pressed: bool, shift: bool, alt: bool, ctrl: bool |                   | Set the new keyboard state (char typed ...)                                                                                                                                                                                                 |
| on_mouse_moved        | x: int, y: int                                              |                   | Set the new mouse position.                                                                                                                                                                                                                 |
| on_mouse_left_click   |                                                             |                   | Down then up on Left button                                                                                                                                                                                                                 |
| on_mouse_right_click  |                                                             |                   | Down then up on Right button.                                                                                                                                                                                                               |
| on_mouse_middle_click |                                                             |                   | Down then up on middle button.                                                                                                                                                                                                              |
| on_mouse_left_down    |                                                             |                   | Left Mouse button down.                                                                                                                                                                                                                     |
| on_mouse_left_up      |                                                             |                   | Left Mouse button up.                                                                                                                                                                                                                       |
| on_mouse_right_down   |                                                             |                   | Right Mouse button down.                                                                                                                                                                                                                    |
| on_mouse_right_up     |                                                             |                   | Right Mouse button up.                                                                                                                                                                                                                      |
| on_mouse_middle_down  |                                                             |                   | Middle Mouse button down.                                                                                                                                                                                                                   |
| on_mouse_middle_up    |                                                             |                   | Middle Mouse button up.                                                                                                                                                                                                                     |
| on_mouse_wheel        | delta: int                                                  |                   | Mouse Wheel.                                                                                                                                                                                                                                |

## How CEF is compiled under Godot ?

The goal of this document is to make you understand the general ideas behind how
this module `gdcef` is compiled (with examples for Window while similar for
other operating systems). The detail design on how guts are working is described
in an other [document](doc/detailsdesign.md) (currently in gestation). For the
details of the implementation, you will have to dive directly inside the CEF
code source, it has lot of comments (not always easy to apprehend at first
lecture). Else, ask questions either in our Discord, or in the `Discussions` or
`Issues` menu of the associated GitHub repository to help improving this
document.

### Environment

The tree structure of the your project can be different from the one depicted in
the next diagram. For this document we have choose:

```
📦YourProject
 ┣ 📂godot-cpp                 ⬅️ Godot C++ API and bindings (cloned)
 ┗ 📂godot-native              ⬅️ Base folder holding native modules (cloned)
   ┗ 📂browser                 ⬅️ Base folder holding native CEF module
     ┣ 📂gdcef                 ⬅️ Code for the CEF module (cloned)
     ┣ 📂subprocess            ⬅️ Code of the CEF sub-process executable (cloned)
     ┗ 📂cef_binary            ⬅️ CEF distribution used to build the dependencies (downloaded)
```

### The Godot C++ binding API (godot-cpp)

The first component, `godot-cpp` folder, must be present before doing *any*
compilation attempt on a Godot module. This folder comes from this
[repo](https://github.com/godotengine/godot-cpp) and contains binding on the
Godot API and allows you to compile your module like if you we were compiling it
directly inside the code source of the Godot editor (see
[here](https://docs.godotengine.org/en/stable/development/cpp/custom_modules_in_cpp.html)
for more information).

*IMPORTANT:* You have to know that contrary than compiling your module directly
inside the `modules` folder of the Godot engine, this method has the drawback,
each time that one of your exported functions is called, to call extra
intermediate functions imposed by the binding layer. In our case this is fine
since CEF fewly triggers the Godot engine. The other point is that methods may
have their name a little changed compared to the official API. Last good point
for us for this project, is the presence of C++ namespace which fix for us a
name conflict on the error enumerators: Godot and CEF using the same error
names, the compiler does not know which one to use. Finally, to make use CEF
natively inside Godot engine would mean to modify directly the Godot code
source, which is more complex than using C++ binding. If you are curious and
read French you can check this
[document](https://github.com/stigmee/doc-internal/blob/master/doc/tuto_modif_godot_fr.md#compilation-du-module-godot-v34-stable)
detailing how we succeeded.

The `godot-cpp` repository should be cloned **recursively** using the
appropriate branch (i.e. do not clone the master as you would end up with
headers for the 4.0 version) : `git clone --recursive -b 3.4
https://github.com/godotengine/godot-cpp`. Recursive cloning will include the
appropriate godot-headers used to generate the C++ bindings and will produce
this kind of message (useless information have been removed for the clarity of
this document):

```
Cloning into 'godot-cpp'...
...
Submodule 'godot-headers' (https://github.com/godotengine/godot-headers) registered for path 'godot-headers'
Cloning into '<Project>\godot-native\godot-cpp/godot-headers'...
...
Submodule path 'godot-headers': checked out 'd1596b939d6c9f5df86655ea617713ef321ad938'
```

The `godot-cpp` folder is automatically compiled by the install script
`build.py` which call a command similar to these lines:

```
cd godot-cpp
scons platform=windows target=release
```

Where [scons](https://scons.org/) is a build system like Makefile but using the
Python interpreter and the build script knowing the operating system, if to
compile in release or debug mode (and more parameters).

### Prebuilt Chromium Embedded Framework (cef_binary)

The second component, `cef_binary` contains the CEF with prebuilt libraries with
the C++ API and some code to compile. These libraries and artifacts are needed
to make the Godot application compilable and working. They are created when this
component is compiled (in fact, compiling the CEF's `cefsimple` example given in
the source is enough). Note that building CEF source code 'from scratch' is too
complex: too long (around 4 hours with a good Ethernet connection, at worst 1
day with poor Ethernet connection), too huge (around 60 and 100 giga bytes on
your disk) and your system shall install plenty of system packages (apt-get).

Since this folder `cef_binary` cannot be directly git cloned, you have to
downloaded, unpacked and renamed from the CEF website
https://cef-builds.spotifycdn.com/index.html in an automatic way. This is done
by our the install script `build.py` which knows your operating system and the
desired CEF version: an inspection inside the CEF's README (if presents) allows
to know if CEF has been previously downloaded or if the version is matching (if
not, this means we wanted to install a different CEF version: the old
`cef_binary` folder is removed and the new one is downloaded, unpacked and
compiled automatically).

To compile CEF, our build script `build.py` will call something similar to the
following lines (but depending on your operating system):

```
cd ./thirdparty/cef_binary
cmake -DCMAKE_BUILD_TYPE=Release .
cmake --build . --config Release
```

The following libraries and artifacts shall copied into the Godot project root
`res://` else Godot will not be able to locate them and will complain about not
being able to load the module dependencies at project startup. The destination
folder is inside its `build` folder (to be created). Those files, for Windows,
are mandatory to correctly startup CEF. Again, the `build.py` will do it for
you, and for other operating system.

```
📦YourProject
 ┗ 📂build
    ┣ 📂locales                      ⬅️ locale-specific resources and strings
    ┃ ┣ 📜en-US.pak                  ⬅️ English
    ┃ ┗ 📜*.pak                      ⬅️ Other countries
    ┣ 📜chrome_elf.dll               ⬅️
    ┣ 📜d3dcompiler_47.dll           ⬅️ Accelerated compositing support libraries
    ┣ 📜libEGL.dll                   ⬅️ Accelerated compositing support libraries
    ┣ 📜libGLESv2.dll                ⬅️ Accelerated compositing support libraries
    ┣ 📜libcef.dll                   ⬅️ main CEF library
    ┣ 📜snapshot_blob.bin            ⬅️ JavaScript V8 initial snapshot
    ┣ 📜v8_context_snapshot.bin      ⬅️ JavaScript V8 initial snapshot
    ┣ 📜icudtl.dat                   ⬅️ Unicode support data
    ┣ 📜chrome_100_percent.pak       ⬅️ Non-localized resources and strings
    ┣ 📜chrome_200_percent.pak       ⬅️ Non-localized resources and strings
    ┗ 📜resources.pak                ⬅️ Non-localized resources and strings
```

For Windows, actual builds are using dynamic library, and default VS solutions
is configured for static compilation. Therefore need to use VS to compile in
Release mode, and you (the build script) will change the compiler mode of the
Release mode from `/MT` to `/MD`, and add the 2 following preprocessor flags:

* `_ITERATOR_DEBUG_LEVEL = 0;`                 under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.
* `_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH`       under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.

Our build script `build.py` will apply a patch before compiling. For Linux it
seems not possible to compile in static, as consequence the `libcef.so` is quite
fat: more than 1 gigabytes which is a factor more than the one for Windows
(probably because this last knows better that Linux which symbol to export).

*IMPORTANT:* since CEF is using some thirdpart libraries under the LGPL licence.
Compiling them as static libraries will contaminate the project under the GPL
licence (which it is not the case when compiled as dynamic libraries). See this
[post](https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11182). In our case
this fine since our project is already under GPL licence.

### CEF secondary process (subprocess)

This executable is needed in order for the CEF to spawn the various CEF
sub-processes (GPU process, render handler...). In CEF, a secondary process is
needed when the CEF initialization function cannot reach or modify the command
line of the application (the `int main(int argc, char* argv[])`) which it is our
case since we do not want to depend on a modified Godot (forked) holding
internally a CEF. We gave a try: modifying Godot code source works but this
becomes too complex to follow evolution of Godot and CEF (since we are not
developing the Godot engine code source). For more information you can read this
[section](https://github.com/stigmee/doc-internal/blob/master/doc/tuto_modif_godot_fr.md#modification-du-main-de-godot-v34-stable).

The detail design on how the both processes talk together is described in this
[document](doc/detailsdesign.md).

The canonical path of the secondary process shall be known by the primary
process (the primary process is explained in the next section). This is our case
since this secondary process will live next to your application binary.

The code source of this secondary process is simply a simple version of the
CEF's `cefsimple` example given in the source is enough. This executable can be
directly used as it and you will have a minimal browser application.

```
📦subprocess
 ┣ 📂src
 ┃ ┣ 📜main.cpp
 ┃ ┗ 📜main.cpp
 ┗ 📜SConstruct
```

To compile this source :

```
cd subprocess
scons target=release platform=windows workspace=$WORKSPACE godot_version=3.4.3-stable -j8
```

The executable will be created as `gdcefSubProcess.exe`. It should be placed
into the appropriate Godot project inside its `build` folder (to be created).
Again the `build.py` will do it for you.

```
📦YourProject
 ┗ 📂build
    ┣ 📜 ...                         ⬅️ CEF libs and artifacts (see previously)
    ┣ 📦YourProject                  ⬅️ YourProject executable
    ┗ 📦gdcefSubProcess              ⬅️ CEF secondary process
```

### CEF native module (gdcef)

This directory contains the source of the gdcef library, allowing to generate
the `libgdcef.dll` module. This dll file can then be loaded by the GDNative
module (see Module configuration). The detail design is described in this
[document](doc/detailsdesign.md).

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
cd gdcef
scons target=release platform=windows -j8 workspace=$WORKSPACE godot_version=3.4.3-stable -j8
```

The library `libgdcef.dll` will be generated into the build directory. It should be placed
into the appropriate Godot project inside its `build` folder (to be created).
Again the `build.py` will do it for you.

```
📦YourProject
 ┗ 📂build
    ┣ 📜 ...                         ⬅️ CEF libs and artifacts (see previously)
    ┣ 📦YourProject                  ⬅️ YourProject executable
    ┣ 📦gdcefSubProcess              ⬅️ CEF secondary process
    ┗ 📜libgdcef.dll                 ⬅️ Our CEF native module library for Godot
```

### Godot module configuration

In order for native modules to be used by Godot, you have to create the
following 2 files under the the Godot project root `res://` (for example in our
case in the folder `libs`) else Godot will not be able to locate them and will
complain about not being able to load the module dependencies at project
startup.

```
📦YourProject                        ⬅️ Godot res://
 ┣ 📜project.godot                   ⬅️ Your Godot project (here YourProject)
 ┣ 📂libs
 ┃ ┣ 📜gdcef.gdns                    ⬅️ CEF native script for gdcef.gdnlib
 ┃ ┗ 📜gdcef.gdnlib                  ⬅️ CEF native script for ../build/libgdcef.dll
 ┗ 📂build
    ┣ 📜 ...                         ⬅️ CEF libs and artifacts (see previously)
    ┣ 📦YourProject                  ⬅️ YourProject executable
    ┣ 📦gdcefSubProcess              ⬅️ CEF secondary process
    ┗ 📜libgdcef.dll                 ⬅️ Our CEF native module library for Godot
```

- gdcef.gdns:
```
[gd_resource type="NativeScript" load_steps=2 format=2]

[ext_resource path="res://libs/gdcef.gdnlib" type="GDNativeLibrary" id=1]

[resource]
resource_name = "gdcef"
class_name = "GDCef"
library = ExtResource( 1 )
script_class_name = "GDCef"
```

This file holds information of the C++ exported class name `GDCef`, its name on
Godot and refers to the second file `gdcef.gdnlib`.

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

This file holds information on how to find your gdnative library and the library
it depends on.

To use the native module inside Godot, ensure libraries are correctly loaded
into your project (open the lib in the Godot editor, make sure GDCef can be
instantiated in GDScript). Then create a `Spatial` node in the scene graph and
attach to it the `gdcef.gdns` file as `NativeScript` (in the language
selector). If Godot is complaining is probably your node is incompatible. Select
the correct node.

![CEFnode](doc/scenegraph/cef.png)

### Update your CEF version

- Check this website https://cef-builds.spotifycdn.com/index.html and select your
desired operating system.
- Copy the desired CEF version **without the name of the operating system**
(for example `100.0.24+g0783cf8+chromium-100.0.4896.127`) and search in the
`build.py` script the line `CEF_VERSION=` and paste the new version.
- Rerun the `build.py` the `cef_binary` folder will be replaced by the new version.

### Installation prerequisites

In case of doubt see the [original project install documentation](https://github.com/stigmee/install).

#### Install Python3 packages

Our `build.py` script is made in **Python3** to be usable for any operating
systems (Linux, MacOS X, Windows). Please do not use Python 2. To make the
installation possible, you will have to install the following python3 modules:

```
# python3 -m pip install scons packaging urllib3 progressbar
python3 -m pip  install -r requirements.txt
```

- `scons` is a Makefile made in Python and it is needed to compile Godot.
- `urllib3` and `packaging` are needed to download and unarchive some tarballs.

#### Install system packages

Install the following tools: `g++`, `ninja`, `cmake` (greater or equal to
3.21.0).

- For Linux, depending on your distribution you can use `sudo apt-get install`.
  To upgrade your cmake you can see this
  [script](https://github.com/stigmee/doc-internal/blob/master/doc/install_latest_cmake.sh).
- For MacOS X you can install [homebrew](https://brew.sh/index_fr).
- For Windows user you will have to install:
  - Visual Studio: https://visualstudio.microsoft.com/en/vs/ (mandatory)
  - Python3: https://www.python.org/downloads/windows/
  - CMake: https://cmake.org/download/
  - Ninja: https://ninja-build.org/
  - Git: https://git-scm.com/download/win

To compile Stigmee for Windows:
- Ensure VS2022 is installed
- Open an **x64 Native Tools Command Prompt for VS 2022**, with
  **Administrator** privilege (this should be available in the start menu under
  Visual Studio 2022). This ensures the environment is correctly set to use the
  VS tools.

### Gallery

Projects interested by / using this module. Please do not hesitate to give your project
links and pictures by pull requests.

- https://elitemeta.city/

Click to see on the image to see the Elitemeta video shared on IPFS.
[![elitemeta](doc/gallery/elitemeta.jpg)](https://ipfs.io/ipfs/QmaL7NY5qs3AtAdcX8vFhqaHwJeTMKfP3PbzcHZBLmo1QQ?filename=elitemeta_0.mp4)
Thanks to the team for having shared this video.
