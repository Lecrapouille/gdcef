# Chromium Embedded Framework Webview for Godot

This repository contains the source code of C++ classes that wrap a subset of the [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) (CEF for short) [API](https://magpcss.org/ceforum/apidocs/). These classes are compiled to be used either as a Godot 3.5 native module (GDNative) or a Godot 4.2 extension (GDExtension), allowing you to implement a webview in your 2D and 3D games through GDScript for Linux, Windows and macOS.

We have named this CEF module/extension `gdcef`. You can download it directly from the Godot Asset Library:
- For Godot 3.5: https://godotengine.org/asset-library/asset/1426
- For Godot 4.2: https://godotengine.org/asset-library/asset/2508

For users who don't want to compile this project themselves, pre-built artifacts are available at https://github.com/Lecrapouille/gdcef/releases

For developers: Since Godot 3.x GDNative and Godot 4.x GDExtension are not compatible, there are two branches:
- https://github.com/Lecrapouille/gdcef/tree/godot-3.x for Godot 3.4 or 3.5 (no longer maintained)
- https://github.com/Lecrapouille/gdcef/tree/godot-4.x for Godot 4.2 and later
- Note: Versions before Godot 3.4, Godot 4.0, and 4.1 have not been tested and are not supported

Developer instructions:
- Use `git checkout godot-3.x` for developing the GDNative version for Godot 3.4 or 3.5
- Use `git checkout godot-4.x` for developing the GDExtension version for Godot 4.2 and later

**Note:** After discussions with some developers who only want to display their JS games without necessarily needing Godot, there is a better alternative: When compiling CEF, the build system also compiles a small CEF application that opens Google by default. You can modify it to launch your game (local or remote) as the default page. See this repository https://github.com/Lecrapouille/exa-application and adapt these [lines](https://github.com/Lecrapouille/exa-application/blob/master/build.py#L47-L48) for your application name (title) and URL.

## Alternative Godot WebView Projects

- In Rust: https://github.com/doceazedo/godot_wry
- In Qt: https://godotwebview.com/
- Godot 3 and Android: https://github.com/Sam2much96/GodotChrome/tree/master