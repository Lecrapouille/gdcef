# Chromium Embedded Framework as Godot 3.4+ native module

This repository contains the source code of some C++ classes wrapping a subset
of the [Chromium Embedded
Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) API into a
Godot 3.4+ native module (GDNative) which allows you to implement a web
browser for your 2D and 3D games through your gdscripts for Linux and for
Windows.  We have named this CEF GDNative module `gdcef`.

## Repository overview

This repository contains the following things:
- C++17 code source for the [primary CEF process](../gdcef/) (your
  Godot application).
- C++17 code source for the [secondary CEF process](../subprocess/)
  (called by the first CEF process).
- A [2D demo](../demos/2D/) and [3D demos](../demos/3D/).
- A python-3 [build script](../build.py) that will git clone the
  Godot-cpp binding, the CEF tarball, compile CEF, compile the primary and
  secondary CEF process, and create the CEF artifacts.
- Documentation.

*Note:* We are using C++17, but we are not using fancy C++ features, we just to
use the 17 because we need `filesystem`.

## Prerequisites for compiling the native module

In case of doubt, you can also read the
[documentation](https://github.com/stigmee/install) for installing the original
project it gives additional information.

### CEF license

**IMPORTANT:** I'm not a jurist but since CEF seems using some third-party
libraries under the LGPL license (see this
[post](https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11182)) and compiling
CEF as a static library will contaminate the project under the GPL license
(which is not the case when compiled as a dynamic library).

In our case, CEF is compiled as a static library for Windows (else we got issues,
see our [patch](../patches/CEF/win/)) and for Linux, since the
`libcef.so` 1 GB, which is heavy, I did not succeed to compile it as static
library to make it smaller.

### Operating System

*IMPORTANT:* Currently this module is only working for Linux and for
Windows. Devices such as Android and IOS are not supported by CEF. This module
can be compiled for macOS but needs some modification in the code source and
extra C# code. I do not have a MacBook full-time. I'm looking for some MacOS
developers to help me (see my branch `dev-darwin`). You can be helped with
https://github.com/CefView/CefViewCore and the
[cefsimple](https://bitbucket.org/chromiumembedded/cef/wiki/Tutorial) example
given within the CEF tarball.

### Install system packages

Install the following tools: `g++`, `ninja`, `cmake` (greater or equal to
3.21.0).

- For Linux, depending on your distribution you can use `sudo apt-get install`.
  To upgrade your cmake you can see this
  [script](https://github.com/stigmee/doc-internal/blob/master/doc/install_latest_cmake.sh).
- For macOS X you can install [homebrew](https://brew.sh/index_fr).
- For Windows users you will have to install:
  - Visual Studio: https://visualstudio.microsoft.com/en/vs/ (mandatory)
  - Python3: https://www.python.org/downloads/windows/
  - CMake: https://cmake.org/download/
  - Ninja: https://ninja-build.org/
  - Git: https://git-scm.com/download/win

To compile GDCef for Windows:
- Ensure VS2022 is installed
- Open an **x64 Native Tools Command Prompt for VS 2022**, with
  **Administrator** privilege (this should be available in the start menu under
  Visual Studio 2022). This ensures the environment is correctly set to use the
  VS tools.

### Install Python3 packages

Our [build.py](../build.py) script is made in **Python3** to be used
for any operating system (Linux, macOS X, Windows). Please do not use
Python 2. Please install the needed Python packages with pip by typing the
following line into a terminal:

```
python3 -m pip install -r requirements.txt
```

## Compilation for Linux and for Windows

### Compilation of the native module for Godot 3.5

The [build.py](../build.py) does not have the command line. It deals with all cases by itself (your
operating system, the number of cores of your CPU ...). By default, it deals with
Godot 3.5.

```
cd addons/gdcef
./build.py
```

Please be patient! The script needs some time for completing its job since it
has:
- to download CEF from this website https://cef-builds.spotifycdn.com/index.html
  (600 MB) and extract the tarball inside `../thirdparty/cef_binary`
  and compile it.
- to git clone [godot-cpp](https://github.com/godotengine/godot-cpp) into
  `../thirdparty/godot-3.5` and compile it.
- to compile the [primary CEF process](../gdcef/).
- to compile the  [secondary CEF process](../subprocess/).
- to extract CEF artifacts (*.so *.pak ...) into `build` folder (at the root of
  this repo).

**Note concerning Linux:**

For the moment, the `libcef.so` (CEF), `libgdcef.so` (Godot native) and other
shared libraries (`libvulkan.so` ...), as artifacts, are not found by the system
(even if indicated in the .gdnlib file. So for the moment, you have to store the
path of the build in your `LD_LIBRARY_PATH` (for example in your `~/.bashrc`
file).

```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/your/path/gdcef/examples/build
```

### Compilation of the native module for Godot 3.4

This module has not been tested with a Godot version lower than 3.4. For Godot
3.4:
- Search in [build.py](../build.py) script the line `GODOT_VERSION =
  "3.5"` and replaced with the desired version.
- Run the build.py like explained for Godot 3.5.

### Update the CEF version

If desired, you can change the CEF version at any moment (even after having
compiled CEF).

- Check this website https://cef-builds.spotifycdn.com/index.html and select
  your desired operating system.
- Copy the desired CEF version **without the name of the operating system** (for
  example `100.0.24+g0783cf8+chromium-100.0.4896.127`) and search in
  [build.py](../build.py) script the line `CEF_VERSION=` and paste the
  new version.
- **Be sure your CEF version contains `+` symbols but not the URL-encode `%2B` format.**
- Rerun the `build.py` the older `thirdparty/cef_binary` folder will be replaced
  automatically by the new version.

### Running demos

Once [build.py](../build.py) script has done success its job, you
can start your Godot editor and goes into `../demos`, and load the
Godot project of demos. They are ready to use. See this
[README](demos/README.md) describing the given demos.

A concrete Godot application using CEF can be found [here](https://github.com/stigmee/stigmee).

## What do I have to do next for using CEF in my personal project?

- Copy the `build/` folder holding CEF artifacts that have been compiled into
  your Godot project.
- Remove the `build/cache` folder if you have used CEF previously.
- Copy and adapt the `gdcef.gdns` and `gdcef.gdnlib` inside your Godot
  project. See more information in the last section of this
  [document](../doc/detailsdesign.md).
- CEF can run from the Godot editor and you can export your project for Linux
  and Windows as usual.
- The gdcef module checks the presence of CEF artifacts and the presence of the
  secondary CEF process.  If they are not present, your application will close.
- In your Godot scene create a `Node` or `Spatial` named for example
  `CEF`. Extend it to be a `GDCEF` by setting the path to `gdcef.gdns` as
  `Nativescript`.
- Create a Godot `TextRect` that will receive your browser texture.
- Create a gdscript and, for example, inside `func _ready():` from the `$CEF`
  node, make create a new browser tab named `browser name` (it will be a Godot
  child node that can be found with a function such as `$CEF.get_node`) and make
  `TextRect` get the texture of your browser tab. See the following code:

```
var browser = $CEF.create_browser("https://github.com/Lecrapouille/gdcef", "browser name", dimension_with, dimension_height)
$TextureRect.texture = browser.get_texture()
```

You should have a minimal CEF browser not reacting to your mouse and key
binding. See the demo 3D to make your browser tab reacts to our input events.

## Diving inside this project

### Change CEF options

Inside the `static void configureCEF(...` function in the
`../gdcef/src/gdcef.cpp` you can modify some options of CEF like the
verbosity, where to generate the cache folder, locales ...

### CEF API

The API for the gdscript is given in this
[document](../doc/API.md). This document will describe the functions
that can be called from your gdscripts.

### Software architecture and details design

- The details design is given in this
  [document](../doc/detailsdesign.md). This document will explain to you
  the reason of the tree organization, how gdcef are compiled, why you need a
  secondary process, ...

- The software architecture is given in this
  [document](../doc/architecture.md). This document explains how CEF
  works internally. **Note: this document is a draft**.

