# Chromium Embedded Framework Webview for Godot

This repository contains the source code of some C++ classes wrapping a subset
of the [Chromium Embedded
Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) (CEF for
shorter) [API](https://magpcss.org/ceforum/apidocs/) compiled to be used either
as a Godot 3.5 native module (GDNative) or Godot 4.2 extension (GDExtension)
which allows you to implement a webview for your 2D and 3D games through your
gdscripts for Linux and Windows (Mac OS should work, if you are a MacOS dev,
please contact me by creating an issue).

We have named this CEF module/extension `gdcef`. It can be downloaded directly
from the Godot asset library:
- For Godot 3.5: https://godotengine.org/asset-library/asset/1426
- For Godot 4.2: https://godotengine.org/asset-library/asset/2508

For people, that does not want to compile this project by their selves can download
pre-build artefacts https://github.com/Lecrapouille/gdcef/releases

For developers, since Godot 3.x GDNative and Godot 4.x GDExtension are not
compatible, there are two branches:
- https://github.com/Lecrapouille/gdcef/tree/godot-3.x for Godot 3.4 or 3.5 (no longer maintained).
- https://github.com/Lecrapouille/gdcef/tree/godot-4.x for Godot 4.2 and later.
- Note: Godot before 3.4, Godot 4.0 and 4.1 versions have not been tested and
  are not be supported.

For developers, you have to do:
- `git checkout godot-3.x` for developing GDNative version for Godot 3.4 or 3.5.
- `git checkout godot-4.x` for developing GDExtension version for Godot 4.2 and later.

**Note:** After talking with some developpers who want only display their JS game
but do not necessary need Godot. In this case there is a better alternative: when compiling CEF,
the build system also compile a small CEF application which, by default, open google. You can
adapt to launch your game (local or remote) as default page. See this
repo https://github.com/Lecrapouille/exa-application and adapt these
[lines](https://github.com/Lecrapouille/exa-application/blob/master/build.py#L47-L48)
for your application name (title) and the URL.