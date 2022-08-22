# Godot/CEF Demos

## Compile demos

If all prerequisites have been installed (Python 3, Godot 3.4+, CMake > 3.19, g++)
and you have either Linux or Windows. Just type:

```
cd ..
./build.py
```

is enough to:
- Download and compile CEF
- Download and compile Godot-cpp
- Create the CEF artifcats needed for running demos (inside `build`).

No command line is needed.

**Workaround For Linux:** for the moment, the `libcef.so` and other shared libraries,
as artifcats, are not found by the systeme (even if indicated in the `.gdnlib` file.
So for the moment, you have to store the path of the `build` in your `LD_LIBRARY_PATH`
(for example in your `~/.bashrc` file).

```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/your/path/gdcef/examples/build
```

**Note concerning the build folder**:

CEF artifcats are searched inside the `build` folder at the root of your Godot project.
Because we have several demos and CEF artifacts are heavy (libcef.so is > 1 GB), to avoid
consuming GB of disk space of duplicated files, we have stored artifacts at `examples/build`
and make demos have an alias on this folder. For your personal project, it's better not
using alias and have all CEF artifacts in a real folder `build` at the root of your Godot
project.

## Run demos

Just open you Godot editor 3.4+ and search the `project.godot`. Open the demo and run it
directly (no prerequesites).

## Demos

## Demo 00: Multiple CEF browsers in 2D

A demo showing a 2D GUI with multiple CEF browser tabs. No mouse and keyboard
events are managed. A timer is loading URLs one by one.

![Screenshot](2D/icon.png)

## Demo 01: Single CEF browser in 3D

A demo showing a 2D GUI with a single CEF browser tab. The tab is rotating inside a 3D scene.
You can use your mouse and keyboard to surf on the web :)

This demo is based on the asset library: https://godotengine.org/asset-library/asset/127

![Screenshot](3D/icon.png)
