# Details design: How CEF is compiled under Godot?

The goal of this document is to make you understand the general idea behind how
this module `gdcef` is compiled (with examples for Windows while similar for
other operating systems). The detailed design of how guts are working is described
in another [document](addons/gdcef/doc/detailsdesign.md) (currently unfinished). For the
details of the implementation, you will have to dive directly inside the CEF
code source, it has a lot of comments (not always easy to apprehend at first
read). Else, ask questions either in the `Discussions` or `Issues` menu of the
associated GitHub repository to help improve this document.

## Environment

The tree structure of your project can be different from the one depicted in
the next diagram. For this document we chose:

```
📦YourProject
 ┣ 📂godot-cpp                 ⬅️ Godot C++ API and bindings (cloned)
 ┗ 📂godot-native              ⬅️ Base folder holding native modules (cloned)
   ┗ 📂browser                 ⬅️ Base folder holding native CEF module
     ┣ 📂gdcef                 ⬅️ Code for the CEF module (cloned)
     ┣ 📂subprocess            ⬅️ Code of the CEF sub-process executable (cloned)
     ┗ 📂cef_binary            ⬅️ CEF distribution used to build the dependencies (downloaded)
```

## The Godot C++ binding API (godot-cpp)

The first component, `godot-cpp` folder, must be present before doing *any*
compilation attempt on a Godot module. This folder comes from this
[repo](https://github.com/godotengine/godot-cpp) and contains binding on the
Godot API allows you to compile your module as if you were compiling it
directly inside the code source of the Godot editor (see
[here](https://docs.godotengine.org/en/stable/development/cpp/custom_modules_in_cpp.html)
for more information).

*IMPORTANT:* You have to know that contrary to compiling your module directly
inside the `modules` folder of the Godot engine, this method has a drawback,
each time that one of your exported functions is called, call extra
intermediate functions imposed by the binding layer. In our case, this is fine
since CEF few triggers the Godot engine. The other point is that methods may
have their name a little changed compared to the official API. The last good point
for us for this project is the presence of C++ namespace which fixes for us a
name conflict on the error enumerators: Godot and CEF use the same error
names, but the compiler does not know which one to use. Finally, to make use of CEF
natively inside Godot engine would mean to directly modify the Godot source
code, which is more complex than using C++ binding. If you are curious and
read French you can check this
[document](https://github.com/stigmee/doc-internal/blob/master/doc/tuto_modif_godot_fr.md#compilation-du-module-godot-v34-stable)
detailing how we succeeded.

The `godot-cpp` repository should be cloned **recursively** using the
appropriate branch (i.e. do not clone the master as you would end up with
headers for the 4.0 version): `git clone --recursive -b 3.4
https://github.com/godotengine/godot-cpp` (this project also works for Godot 3.5).
Recursive cloning will include the appropriate godot-headers used to generate the
C++ bindings and will produce this kind of message (useless information have been
removed for the clarity of this document):

```
Cloning into 'godot-cpp'...
...
Submodule 'godot-headers' (https://github.com/godotengine/godot-headers) registered for path 'godot-headers'
Cloning into '<Project>\godot-native\godot-cpp/godot-headers'...
...
Submodule path 'godot-headers': checked out 'd1596b939d6c9f5df86655ea617713ef321ad938'
```

The `godot-cpp` folder is automatically compiled by the install script
`build.py` which calls a command similar to these lines:

```
cd godot-cpp
scons platform=windows target=release
```

Where [scons](https://scons.org/) is a build system like Makefile but using the
Python interpreter and the build script knowing the operating system, if it has
to compile in release or debug mode (and more parameters).

## Prebuilt Chromium Embedded Framework (cef_binary)

The second component, `cef_binary` contains the CEF with prebuilt libraries with
the C++ API and some code to compile. These libraries and artifacts are needed
to make the Godot application compilable and working. They are created when this
component is compiled (in fact, compiling the CEF's `cefsimple` example given in
the source is enough). Note that building CEF source code 'from scratch' is too
complex: too long (around 4 hours with a good Ethernet connection, at worst 1
day with poor Ethernet connection), too huge (around 60 and 100 GB on
your disk) and your system shall install plenty of system packages (apt-get).

Since this folder `cef_binary` cannot be directly git cloned, you have to
download, unpack and rename it from the CEF website
https://cef-builds.spotifycdn.com/index.html in an automatic way. This is done
by our the install script `build.py`, which knows your operating system and the
desired CEF version: an inspection inside the CEF's README (if present) allows
to know if CEF has been previously downloaded or if the version is matching (if
not, this means we wanted to install a different CEF version: the old
`cef_binary` folder is removed and the new one is downloaded, unpacked and
compiled automatically).

To compile CEF, our build script `build.py` will call something similar to the
following lines (but depending on your operating system):

```
cd ./thirdparty/cef_binary
cmake -DCMAKE_BUILD_TYPE=Release .
cmake --build . --config Release
```

The following libraries and artifacts are copied into the Godot project root
`res://`, else Godot will not be able to locate them and will complain about not
being able to load the module dependencies at project startup. The destination
folder is inside its `build` folder (to be created). Those files, for Windows,
are mandatory to correctly startup CEF. Again, the `build.py` will do it for
you, and for other OSes.

```
📦YourProject
 ┗ 📂build
    ┣ 📂locales                      ⬅️ locale-specific resources and strings
    ┃ ┣ 📜en-US.pak                  ⬅️ English
    ┃ ┗ 📜*.pak                      ⬅️ Other countries
    ┣ 📜chrome_elf.dll               ⬅️
    ┣ 📜d3dcompiler_47.dll           ⬅️ Accelerated compositing support libraries
    ┣ 📜libEGL.dll                   ⬅️ Accelerated compositing support libraries
    ┣ 📜libGLESv2.dll                ⬅️ Accelerated compositing support libraries
    ┣ 📜libcef.dll                   ⬅️ main CEF library
    ┣ 📜snapshot_blob.bin            ⬅️ JavaScript V8 initial snapshot
    ┣ 📜v8_context_snapshot.bin      ⬅️ JavaScript V8 initial snapshot
    ┣ 📜icudtl.dat                   ⬅️ Unicode support data
    ┣ 📜chrome_100_percent.pak       ⬅️ Non-localized resources and strings
    ┣ 📜chrome_200_percent.pak       ⬅️ Non-localized resources and strings
    ┗ 📜resources.pak                ⬅️ Non-localized resources and strings
```

For Windows, actual builds are using dynamic libraries, and default VS solutions
is configured for static compilation. Therefore need to use VS to compile in
Release mode, and you (the build script) will change the compiler mode of the
Release mode from `/MT` to `/MD`, and add the 2 following preprocessor flags:

* `_ITERATOR_DEBUG_LEVEL = 0;`                 under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.
* `_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH`       under `C/C++ >> Preprocessor >> PreprocessorDefinitions`.

Our build script `build.py` will apply a patch before compiling. For Linux it
seems not possible to compile in static, as a consequence the `libcef.so` is quite
fat: more than 1 gigabyte which is a factor more than the one for Windows
(probably because this last knows better than Linux which symbol to export).

*IMPORTANT:* since CEF is using some third-party libraries under the LGPL license.
Compiling them as static libraries will contaminate the project under the GPL
license (which is not the case when compiled as dynamic libraries). See this
[post](https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11182). In our case,
this is fine since our project is already under GPL license.

## CEF secondary process (subprocess)

This executable is needed in order for the CEF to spawn the various CEF
sub-processes (GPU process, render handler...). In CEF, a secondary process is
needed when the CEF initialization function cannot reach or modify the command
line of the application (the `int main(int argc, char* argv[])`) which is our
case since we do not want to depend on a modified Godot (forked) holding
internally a CEF. We gave it a try: modifying Godot source code works but this
becomes too complex to follow the evolution of Godot and CEF (since we are not
developing the Godot engine source code). For more information, you can read this
[section](https://github.com/stigmee/doc-internal/blob/master/doc/tuto_modif_godot_fr.md#modification-du-main-de-godot-v34-stable).

The detailed design on how both processes talk together is described in this
[document](addons/gdcef/doc/detailsdesign.md).

The canonical path of the secondary process shall be known by the primary
process (the primary process is explained in the next section). This is our case
since this secondary process will live next to your application binary.

The code source of this secondary process is simply a simple version of the
CEF's `cefsimple` example given in the source is enough. This executable can be
directly used as it and you will have a minimal browser application.

```
📦subprocess
 ┣ 📂src
 ┃ ┣ 📜main.cpp
 ┃ ┗ 📜main.cpp
 ┗ 📜SConstruct
```

To compile this source :

```
cd subprocess
scons target=release platform=windows workspace=$WORKSPACE godot_version=3.4.3-stable -j8
```

The executable will be created as `gdcefSubProcess.exe`. It should be placed
into the appropriate Godot project inside its `build` folder (to be created).
Again the `build.py` will do it for you.

```
📦YourProject
 ┗ 📂build
    ┣ 📜 ...                         ⬅️ CEF libs and artifacts (see previously)
    ┣ 📦YourProject                  ⬅️ YourProject executable
    ┗ 📦gdcefSubProcess              ⬅️ CEF secondary process
```

## CEF native module (gdcef)

This directory contains the source of the gdcef library, allowing to generate
the `libgdcef.dll` module. This dll file can then be loaded by the GDNative
module (see Module configuration). The detail design is described in this
[document](addons/gdcef/doc/detailsdesign.md).

```
📦gdcef
 ┣ 📂src
 ┃ ┣ 📜gdcef.cpp
 ┃ ┣ 📜gdcef.hpp
 ┃ ┣ 📜gdbrowser.cpp
 ┃ ┣ 📜gdbrowser.hpp
 ┃ ┣ 📜gdlibrary.cpp
 ┃ ┗ 📜...
 ┗ 📜SConstruct
```

To compile this source :

```
cd gdcef
scons target=release platform=windows -j8 workspace=$WORKSPACE godot_version=3.4.3-stable -j8
```

The library `libgdcef.dll` will be generated into the build directory. It should be placed
into the appropriate Godot project inside its `build` folder (to be created).
Again the `build.py` will do it for you.

```
📦YourProject
 ┗ 📂build
    ┣ 📜 ...                         ⬅️ CEF libs and artifacts (see previously)
    ┣ 📦YourProject                  ⬅️ YourProject executable
    ┣ 📦gdcefSubProcess              ⬅️ CEF secondary process
    ┗ 📜libgdcef.dll                 ⬅️ Our CEF native module library for Godot
```

## Godot module configuration

In order for native modules to be used by Godot, you have to create the
following 2 files under the Godot project root `res://` (for example in our
case in the folder `libs`) else Godot will not be able to locate them and will
complain about not being able to load the module dependencies at a project
startup.

```
📦YourProject                        ⬅️ Godot res://
 ┣ 📜project.godot                   ⬅️ Your Godot project (here YourProject)
 ┣ 📂libs
 ┃ ┗ 📜gdcef.gdextension             ⬅️ Godot know what dynamic libraries should be loaded for each platform
 ┗ 📂build
    ┣ 📜 ...                         ⬅️ CEF libs and artifacts (see previously)
    ┣ 📦YourProject                  ⬅️ YourProject executable
    ┣ 📦gdcefSubProcess              ⬅️ CEF secondary process
    ┗ 📜libgdcef.dll                 ⬅️ Our CEF native module library for Godot
```

This file holds information on the C++ exported class name `GDCef`, its name on
Godot and refers to the second file `gdcef.gdextension`.

- gdcef.gdextension:
```
[configuration]
entry_symbol = "gdcef_library_init"
compatibility_minimum = 4.1

[libraries]
linux.x86_64.debug = "res://cef_artifacts/libgdcef.so"
linux.x86_64.release = "res://cef_artifacts/libgdcef.so"
linux.x86_32.debug = "res://cef_artifacts/libgdcef.so"
linux.x86_32.release = "res://cef_artifacts/libgdcef.so"

windows.x86_64.debug = "res://cef_artifacts/libgdcef.dll"
windows.x86_64.release = "res://cef_artifacts/libgdcef.dll"
windows.x86_32.debug = "res://cef_artifacts/libgdcef.dll"
windows.x86_32.release = "res://cef_artifacts/libgdcef.dll"
```

This file lets Godot know what dynamic libraries should be loaded for each platform
and the entry function for the module.

To use the native module inside Godot, ensure libraries are correctly loaded
into your project. Then create a `GDCEF` node in the scene graph.

![CEFnode](scenegraph/cef.png)

**Beware:** In Linux, you will have to write something like:
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/your/path/gdcef/examples/build
```

to make your system finds shared libraries such as `libcef.so`.

