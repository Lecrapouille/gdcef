# Software Architecture.

**WIP: Consider this document as a draft!!!**

This details design document is about [gdcef]
(https://github.com/Lecrapouille/gdcef) implementing a [Chromium Embedded
Framework](https://bitbucket.org/chromiumembedded/cef/wiki/Home) (CEF) native
module for the [Godot editor](https://godotengine.org/) (gdCef).

Inside the `gdcef` folder, two main classes deriving from `godot::Node` have
been created to wrap the CEF C++ API to be usable from Godot scripts. Derived
from Godot Nodes, it allows instances of these classes to be attached inside to
the scene-graph as depicted by the following picture.

![CEFnode](scenegraph/cef.png)

See this
[document](https://docs.godotengine.org/en/stable/classes/class_node.html)
concerning what a Godot Node is.

## Classes Diagram

The following picture depicts the class diagram:

![classdiag](architecture/classes.png)

- `GDCef` implemented in [gdcef/src/gdcef.cpp](gdcef.[ch]pp). Its goal is to
  wrap up the initialization phase of CEF, its settings, and the loopback of
  messages of CEF subprocesses. This class allows creating `GDBrowserView`
  that are attached as child nodes inside the scene-graph.

- `GDBrowserView` implemented in [gdcef/src/gdbrowser.cpp](gdbrowser.[ch]pp).
  Its goal is to wrap a browser view allowing to display the web document, to
  interact with the user (mouse, keyboard), to load pages, ...

The [gdcef/src/gdlibrary.cpp](gdcef/src/gdlibrary.cpp) allows Godot to
register the two classes. See this document for more information:
https://docs.godotengine.org/en/stable/development/cpp/custom_modules_in_cpp.html

## CEF Secondary process

A [secondary CEF
process](https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage.md#markdown-header-separate-sub-process-executable)
is needed when CEF (here, our class `GDCef`) cannot directly access the
`main(int argc, char* argv[])` function of the application. This is mandatory for
its initialization.
<!---
This is, unfortunately, the case since CEF is created as a node
scene-graph but CEF does not come natively inside the Godot engine and
accessing the Godot engine `main` function.
-->
This is, unfortunately, the case since CEF is created as a Node scene-graph
but CEF and its access to Godot Engine's `main` function do not exist
in Godot's source code.

When starting, CEF will fork the application several times into processes
and the forked processes become specialized processes

You have to know that CEF modifies the content of your `argv` and this may mess
up your application if it also parses the command line (you can back it up,
meaning using a `std::vector` to back up `argv` and after CEF init to restore
values in `argv` back). What is "two separated processes" exactly? Just an extra
fork: the main process forks itself and calls the secondary process, which can
fully access it is own main(int argc, char* argv[]). The main constraint is the
path of the secondary process shall be canonic (and this is a pain to get the
real path).

<!---
## Diagram sequence
-->

