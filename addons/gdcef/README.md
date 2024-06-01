# Chromium Embedded Framework as Godot 4.2 native module

This repository contains the source code of some C++ classes wrapping a subset
of the [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home)
API into a Godot 4.2 native module (GDExtension) which allows you to
implement a web browser for your 2D and 3D games through your gdscripts for
Linux and Windows. We have named this CEF GDExtension module `gdcef`.

## Installation steps

The complete guide for compiling this project with the Python3 build script for Linux and Windows
is given [here](doc/installation.md). It also explains how to update the CEF version.

For busy people, here are the direct steps:

```
cd addons/gdcef
python3 -m pip install -r requirements.txt
python3 build.py
```

If successful, a `build` folder at the root of the project shall have been created.
It holds all CEF/Godot artifacts. Use this folder for your Godot project and do not
forget to add `.gdns` and `.gdnlib` files to refer `libgdcef.so` or `libgdcef.dll`
like done for any Godot modules.

## Repository overview

This repository contains the following things:
- C++17 code source for the [primary CEF process](gdcef/) (your
  Godot application).
- C++17 code source for the [secondary CEF process](subprocess/)
  (called by the first CEF process).
- A [2D demo](demos/2D/) and [3D demo](demos/3D/).
- A python-3 [build script](build.py) that will git clone the
  Godot-cpp binding, download the CEF tarball, extract it, compile CEF, compile the primary and
  secondary CEF process, and finally create the CEF artifacts in the `build` folder.

*Note:* We are using C++17, but we are not using fancy C++ features, we just
use C++17 because we need `filesystem`.

## Documentation

We also provide some documents to help you understanding the nuts and bolts of this project:

- The Godot gdscript API is given in this [document](doc/API.md).
  This document will describe the functions that can be called from your gdscripts.

- The design details are given in this
  [document](doc/detailsdesign.md). This will explain to you
  the reason of the tree organization, how gdcef is compiled, why you need a
  secondary process, ...

- The software architecture is given in this
  [document](doc/architecture.md). This document explains how CEF
  works internally. **Note: this document is a draft**.

## Running demos

Once the compilation of this project has ended with success, you
can start your Godot editor 3.5 and go into the [demos folder]](demos), and try the
2D and 3D example. They are ready to use. See this [README](demos/README.md)
describing the given demos.

## What do I have to do next for using CEF in my personal project?

- Copy the `build/` folder holding CEF artifacts that have been compiled into
  your Godot project.
- Remove the `build/cache` folder if you have used CEF previously.
- In the case you dislike the folder name holding CEF artifacts `build`, you can change it.
  Search in [build.py](../build.py) script the line `CEF_ARTIFACTS_FOLDER = "build"`
  and rename it. Rerun the `build.py`. This will force Godot knowing default path.
  Else you can refer it explictely when calling `initialize({"artifacts": "res://cef_artifacts/", ... })`.
- Copy and adapt the `gdcef.gdns` and `gdcef.gdnlib` inside your Godot
  project and adapt the path to shared libraries. See more information in the last
  section of this [document](doc/detailsdesign.md).
- CEF can run from the Godot editor and you can export your project for Linux
  and Windows as usual.
- The gdcef module checks the presence of CEF artifacts and the presence of the
  secondary CEF process.  If they are not present, your application will close.
- In your Godot scene create a `Node` or `Spatial` named for example
  `CEF`. Extend it to be a `GDCEF` by setting the path to `gdcef.gdns` as
  `Nativescript`.
- Follow this [document](doc/API.md) describe the functions that can be called from your gdscripts.
- Create a Godot `TextRect` that will receive your browser texture.
- When initializing CEF with `initialize` instead of the regular Godot `_init`.
- Create a gdscript and, for example, inside `func _ready():` from the `$CEF`
  node, make create a new browser with function `create_browser` taking the desired URL,
  the `TextRect` and optional options. The created browser will be a Godot
  child node with default name `browser_<id>` (with id a number starting from 0). You can give
  a new name with the `get_name()` function. The browser has any Godot node can be found with
  a function such as `$CEF.get_node("browser_0")`.

```
var browser = $CEF.create_browser("https://github.com/Lecrapouille/gdcef", $TextureRect, {})
browser.get_name("hello")
```

- You should obtain a minimal CEF browser not reacting to your mouse and key
binding. See the 2D and 3D demos to make your browser tab reacts to your input events.

When exporting your project, Godot generates your binary application inside
the `build` folder.

## Important note about some architectures

- CEF is working with MacOS but not this current Godot module. If you are a MacOS developper
  you can help me to make this module functional for Mac.
- CEF is not working for IOS and Android devices!
- Chrome extensions limited to version 2 but now everybody uses the version 3.

## Important note about the CEF License

**IMPORTANT:** I'm not a jurist but since CEF seems using some third-party
libraries under the LGPL license (see this
[post](https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11182)) and compiling
CEF as a static library will contaminate **your** project under the GPL license
(which is not the case when compiled as a dynamic library), will force you
to share the source code of your application.

In our case, CEF is compiled as a static library for Windows (else we got issues,
see our [patch](patches/CEF/win/)) and for Linux it is a shared library (`libcef.so`
1 GB, which is heavy). I did not succeed in compiling it as a static library to make it
smaller.
