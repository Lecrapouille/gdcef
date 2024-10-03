# Chromium Embedded Framework as Godot 4.3 native module

This repository provides C++ classes that wrap part of the [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) (CEF) API into a native Godot > 4.2 module (GDExtension), enabling the integration of a web browser into your 2D and 3D games via GDScript on Linux and Windows. We have named this GDExtension module `gdcef`.

## TLDR: Compilation steps

The complete guide for compiling this project with the Python3 build script for Linux and Windows is given [here](doc/installation.md). It also explains how to update the CEF version. For busy people, the direct way is to follow these steps:

```
cd addons/gdcef
python3 build.py
```

If successful, a build folder named `cef_artifacts` at the root of the project shall have been created. It holds all CEF/Godot artifacts. Use this folder for your Godot project. You do not have to add `.gdextension` files refering to `libgdcef.so` or `libgdcef.dll` since automatically added.

## Running demos

After successfully compiling the project, open Godot Editor 4 and navigate to the [demos folder](demos) to try the 2D and 3D demos, which are ready to use. Refer to this [README](demos/README.md) for more details on the demos.

![CEFdemos](doc/pics/demos.png)

## Repository overview

This repository contains the following important elements:
- C++ code source for the [primary CEF process](gdcef/) (your
  Godot application).
- C++ code source for the [secondary CEF process](subprocess/)
  (called by the first CEF process).
- Some demos in [2D](demos/2D/) and [3D](demos/3D/). The 2D demo shows almost all the API.
- A python-3 [build script](build.py) that will git clone the
  Godot-cpp binding, download the CEF tarball, extract it, compile CEF, compile the primary and
  secondary CEF process, and finally create the CEF artifacts in the `build` folder.

*Note:* While we are using C++17, we aren't utilizing advanced features, but we require this version for the `filesystem` library.

## Documentation

We've included documentation to help you understand the core aspects of this project:
- The Godot gdscript API is given in this [document](doc/API.md).
  This document will describe the functions that can be called from your gdscripts.
- The design details are given in this
  [document](doc/detailsdesign.md). This will explain to you
  the reason of the tree organization, how gdcef is compiled, why you need a
  secondary process, ...
- The software architecture is given in this
  [document](doc/architecture.md). This document explains how CEF
  works internally. **Note: this document is a draft**.

## FAQ

### How do I use CEF in my personal project?

- Copy the `cef_artifacts` folder holding CEF artifacts that have been compiled into your Godot project.
- Delete the `cef_artifacts/cache` folder if you have previously used gdCEF.
- If you'd prefer a different name for the folder containing the CEF artifacts, you can change it. In the [build.py](../build.py) script, find the line `CEF_ARTIFACTS_FOLDER_NAME = "cef_artifacts"`, rename it and rerun the `build.py`. This will update the path in Godot.
  Else you can refer it explictely when calling `initialize({"artifacts": "res://cef_artifacts/", ... })`.
- CEF can run directly from the Godot editor, and you can export your project for Linux and Windows as usual.
- The gdcef module verifies the presence of both CEF artifacts and the secondary CEF process. If either is missing, your application will close.
- From the node selector, add in your scenegraph a Godot `TextRect` to hold your browser's texture. Let it name it `TextureRect`.
- From the node selector, find for the `GDCEF` node, let name it `CEF`. If not found that mean the gdextension file has not be loaded.
- Create a gdscript. Use `initialize` to start CEF, instead of the usual Godot `_init` method. Refer to this [document](doc/API.md) for details on the functions that can be called from your GDScripts. For example, inside `func _ready():` from the `$CEF` node, make create a new browser with function `create_browser` taking the desired URL, the `TextRect` and optional options (default: `{}` read the API for knowing possible options). The browser you create will be a child node in Godot, with a default name of `browser_<id>` (where `<id>` starts at 0). You can rename it using the `get_name()` function. The browser has any Godot node can be found with a function such as `$CEF.get_node("browser_0")`.

```
var browser = $CEF.create_browser("https://github.com/Lecrapouille/gdcef", $TextureRect, {})
browser.get_name("hello")
```

- You will get a basic CEF browser that does not respond to mouse or keyboard inputs. Check the 2D and 3D demos to learn how to make your browser respond to input events.
- Here are some projects that might inspire you:
  - https://github.com/face-hh/wattesigma

### Why is my CPU usage at 70% when running gdCEF?

Try switching Godot's graphics mode to 'Compatibility' instead of 'Forward+'. See below:

![graphic mode](doc/pics/graphic_mode.png)

### Important note about certain architectures

- CEF works on macOS, but this Godot module is not yet compatible. If you're a macOS developer, your help in making this module functional on macOS would be appreciated.
- CEF is currently not supported on iOS or Android devices. For Android, you can see this [project](https://github.com/Sam2much96/GodotChrome). 
- Chrome extensions are limited to version 2, although most users now rely on version 3.

### Important notes on the CEF License

**IMPORTANT:** I'm not a legal expert, but be aware that CEF uses some third-party libraries under the LGPL license (see this
[post](https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11182)). Compiling CEF as a static library may subject **your** project to the GPL license, requiring you to share your application's source code. This does not apply when compiling CEF as a dynamic library.

In our case, CEF is compiled as a static library for Windows (due to various issues, see our [patch](patches/CEF/win/)), and as a shared library (`libcef.so` > 1 GB, which is quite large) for Linux. Unfortunately, I was unable to compile it as a static library on Linux to reduce its size.
