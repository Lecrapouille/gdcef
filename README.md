# Chromium Embedded Framework Webview for Godot

This repository contains the source code of some C++ classes wrapping a subset
of the [Chromium Embedded
Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) (CEF for
shorter) [API](https://magpcss.org/ceforum/apidocs/) compiled to be used either
as a Godot 3.5 native module (GDNative) or Godot 4.2 extension (GDExtension)
which allows you to implement a webview for your 2D and 3D games through your
gdscripts for Linux and Windows (Mac OS could work, but help is needed).

We have named this CEF module/extension `gdcef`. It can be downloaded directly
from the Godot asset library:
- For Godot 3.5: https://godotengine.org/asset-library/asset/1426
- For Godot 4.2: https://godotengine.org/asset-library/asset/2508

For developers, since Godot 3.x GDNative and Godot 4.x GDExtension are not
compatible, there are two branches:
- https://github.com/Lecrapouille/gdcef/tree/godot-3.x for Godot 3.4 or 3.5.
- https://github.com/Lecrapouille/gdcef/tree/godot-4.x for Godot 4.2 and later.
- Note: Godot before 3.4, Godot 4.0 and 4.1 versions have not been tested and
  are not be supported.

For developers, you have to do:
- `git checkout godot-3.x` for developing GDNative version for Godot 3.4 or 3.5.
- `git checkout godot-4.x` for developing GDExtension version for Godot 4.2 and later.