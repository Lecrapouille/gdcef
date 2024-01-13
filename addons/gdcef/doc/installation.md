# Chromium Embedded Framework as Godot 4.2+ GDExtension

This repository contains the source code of some C++ classes wrapping a subset
of the [Chromium Embedded
Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) API into a
Godot 4.2+ GDExtension which allows you to implement a web
browser for your 2D and 3D games through your gdscripts for Linux and for
Windows.  We have named this CEF GDExtension `gdcef`.

## Prerequisites for compiling the GDExtension

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
3.21.0). We are using C++17, but we are not using fancy C++ features, we just to
use the 17 because we need `filesystem`.

- For Linux, depending on your distribution you can use `sudo apt-get install`.
  To upgrade your cmake you can see this
  [script](https://github.com/stigmee/doc-internal/blob/master/doc/install_latest_cmake.sh).
- For macOS X you can install [homebrew](https://brew.sh/index_fr).
- For Windows users you will have to install:
  - Visual Studio: https://visualstudio.microsoft.com/en/vs/ (mandatory). Do not forget to
    install Windows SDK (i.e. 10.0.20348.0) in Visual Studio.
  - Python3: https://www.python.org/downloads/windows/
  - CMake: https://cmake.org/download/
  - Ninja: https://ninja-build.org/
  - Git: https://git-scm.com/download/win
  - *Note:* I have installed them for their official website, I did not tried to install them
    from the `winget` command.

To compile GDCef for Windows:
- Ensure VS2022 is installed.
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

**Note concerning Linux:**

For the moment, once the compilation done, the `libcef.so` (CEF), `libgdcef.so`
(Godot native) and other shared libraries (`libvulkan.so` ...), as artifacts,
are not found by the system (even if indicated in the .gdnlib file. So for the
moment, you have to store the path of the build in your `LD_LIBRARY_PATH` (for
example in your `~/.bashrc` file).

```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/your/path/gdcef/examples/build
```

### Compilation of the GDExtension for Godot 3.4+

This module is not compatible with Godot 3.X.
See the [dev-godot-3 branch](https://github.com/Lecrapouille/gdcef/blob/master/addons/gdcef/build.py)
instead.

### Compilation of the GDExtension for Godot 4.2+

This module is not compatible with Godot 4.0 and 4.1.

The [build.py](../build.py) does not have the command line. It deals with all cases by itself (your
operating system, the number of cores of your CPU ...). By default, it deals with
Godot 4.2.

```
cd addons/gdcef
./build.py
```

Please be patient! The script needs some time for completing its job since it
has:
- to download CEF from this website https://cef-builds.spotifycdn.com/index.html
  (+600 MB) and extract the tarball inside `../thirdparty/cef_binary`
  and compile it.
- to git clone [godot-cpp](https://github.com/godotengine/godot-cpp) into
  `../thirdparty/godot-4.2` and compile it.
- to compile the [primary CEF process](../gdcef/).
- to compile the  [secondary CEF process](../subprocess/).
- to extract CEF artifacts (*.so *.pak ...) into `build` folder (at the root of
  this repo).

## Update the CEF version

If desired, you can change the CEF version at any moment (even after having
compiled CEF).

- Check this website https://cef-builds.spotifycdn.com/index.html and select
  your desired operating system.
- Copy the desired CEF version **without the name of the operating system** (for
  example `120.2.4+gc129304+chromium-120.0.6099.199`) and search in
  [build.py](../build.py) script the line `CEF_VERSION=` and paste the
  new version.
- **Be sure your CEF version contains `+` symbols but not the URL-encode `%2B` format.**
- Rerun the `build.py` the older `thirdparty/cef_binary` folder will be replaced
  automatically by the new version.

## What to do if I dislike the folder name `build` holding CEF artifacts ?

You can change it! Search in [build.py](../build.py) script the line
`CEF_ARTIFACTS_FOLDER = "build"` and modifiy it. Rerun the `build.py` and adapt
gdnlib files in `demos/*/libs/` folders. This method will force Godot knowing
default path. Alternatively, adapt the code of your gdscript for
passing the path (see the API for more information):

```
$CEF.initialize({"artifacts": "res://cef_artifacts/", ... })
```
