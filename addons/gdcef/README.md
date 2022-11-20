# Chromium Embedded Framework as Godot 3.5 native module

This repository contains the source code of some C++ classes wrapping a subset
of the [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home)
API into a Godot 3.4 or Godot 3.5 native module (GDNative) which allows you to
implement a web browser for your 2D and 3D games through your gdscripts for
Linux and Windows. We have named this CEF GDNative module `gdcef`.

## Installation steps

See this complete [guide](doc/installation.md) for compiling this
project with the Python3 build script for Linux and Windows. It also explains how
to update the CEF version.

For hurry people, here are direct steps:

```
cd addons/gdcef
python3 -m pip install -r requirements.txt
./build.py
```

This will generate artifacts in the `build/` folder. Use this folder for your Godot
project and add gdns and gdnlib files to refer `libgdcef.so` or `libgdcef.dll`.

## Repository overview

This repository contains the following things:
- C++17 code source for the [primary CEF process](gdcef/) (your
  Godot application).
- C++17 code source for the [secondary CEF process](subprocess/)
  (called by the first CEF process).
- A [2D demo](demos/2D/) and [3D demo](demos/3D/).
- A python-3 [build script](build.py) that will git clone the
  Godot-cpp binding, the CEF tarball, compile CEF, compile the primary and
  secondary CEF process, and create the CEF artifacts.

*Note:* We are using C++17, but we are not using fancy C++ features, we just to
use the 17 because we need `filesystem`.

## Documentation

We give some documents to help you understanding guts of this project:

- The details design is given in this
  [document](doc/detailsdesign.md). This document will explain to you
  the reason of the tree organization, how gdcef are compiled, why you need a
  secondary process, ...

- The software architecture is given in this
  [document](doc/architecture.md). This document explains how CEF
  works internally. **Note: this document is a draft**.

- The API for the gdscript is given in this [document](doc/API.md).
  This document will describe the functions that can be called from your gdscripts.

## Running demos

Once the compilation of this project has ended with success, you
can start your Godot editor 3.5 and goes into the `demos` folder, and try the
2D demonstration and the 3D demonstration. They are ready to use. See this
[README](demos/README.md) describing the given demos.

## What do I have to do next for using CEF in my personal project?

- Copy the `build/` folder holding CEF artifacts that have been compiled into
  your Godot project.
- Remove the `build/cache` folder if you have used CEF previously.
- If you dislike the folder name `build` holding CEF artifacts, you can change it.
  Search in [build.py](../build.py) script the line `CEF_ARTIFACTS_FOLDER = "build"`
  and modifiy it. Rerun the `build.py`. This will force Godot knowing default path.
  Else you can refer it explictely when calling `initialize()`.
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
- When initializing CEF with `initialize` instead of Godot `_init`.
- Create a Godot `TextRect` that will receive your browser texture.
- Create a gdscript and, for example, inside `func _ready():` from the `$CEF`
  node, make create a new browser tab named `browser name` (it will be a Godot
  child node that can be found with a function such as `$CEF.get_node`) and make
  `TextRect` get the texture of your browser tab. See the following code:

```
var browser = $CEF.create_browser("https://github.com/Lecrapouille/gdcef", "browser name", dimension_with, dimension_height)
$TextureRect.texture = browser.get_texture()
```

You should obtain a minimal CEF browser not reacting to your mouse and key
binding. See the demo 3D to make your browser tab reacts to our input events.

When exporting your project, make Godot generates your binary application inside
the `build` folder.

## Important note about the CEF License

**IMPORTANT:** I'm not a jurist but since CEF seems using some third-party
libraries under the LGPL license (see this
[post](https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11182)) and compiling
CEF as a static library will contaminate **your** project under the GPL license
(which is not the case when compiled as a dynamic library). This will force you
to shared the code source of your application.

In our case, CEF is compiled as a static library for Windows (else we got issues,
see our [patch](patches/CEF/win/)) and for Linux it is a shared library (`libcef.so`
1 GB, which is heavy). I did not succeed to compile it as static library to make it
smaller.
