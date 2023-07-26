# Chromium Embedded Framework as Godot 3.5 native module

This repository contains the source code of some C++ classes wrapping a subset
of the [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home)
API into a Godot 3.4 or Godot 3.5 native module (GDNative) which allows you to
implement a web browser for your 2D and 3D games through your gdscripts for
Linux and Windows. We have named this CEF GDNative module `gdcef`.

This module can be downloaded directly from the Godot asset library:

https://godotengine.org/asset-library/asset/1426

*Note:* This current repository is a fork of [the following
repo](https://github.com/stigmee/gdnative-cef) (under GPLv3) with a more
permissive license (MIT). Since the original repo is no longer maintained
by their two original authors (Alain and Quentin), we, the undersigned Alain
and Quentin, gave consent to relicensing the original code under the
MIT license.

## Documentation

Since this current README is not taken when importing this module from the Godot asset
library, all details are referred in the following
[document](addons/gdcef/README.md) which explains you how to build
this module, how to run demos, describe the architecture, details design and
more ...

## TL;DR: Installation

To compile this project with the Python3 build script for Linux and Windows:

```
cd addons/gdcef
python3 -m pip install -r requirements.txt
./build.py
```

This will generate artifacts in the `build/` folder. Use this folder for your Godot
project and add gdns and gdnlib files to refer `libgdcef.so` or `libgdcef.dll`.

## Gallery

If you are using this project, please do not hesitate to share your project links and
pictures by making a GitHub pull request, I'll add them in this gallery. Thanks to
teams for having shared the current gallery:

- https://elitemeta.city

[![elitemeta](addons/gdcef/doc/gallery/elitemeta.jpg)](https://ipfs.io/ipfs/QmaL7NY5qs3AtAdcX8vFhqaHwJeTMKfP3PbzcHZBLmo1QQ?filename=elitemeta_0.mp4)

*Click to see on the image to see the Elitemeta video shared on IPFS.*
