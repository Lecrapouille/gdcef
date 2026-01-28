# Getting Started with gdCEF

This guide explains how to integrate gdCEF into your Godot project.

## Prerequisites

You need either:

- Compiled CEF artifacts from running `build.py`
- Or prebuilt artifacts from [GitHub Releases](https://github.com/Lecrapouille/gdcef/releases)

## Step 1: Add CEF Artifacts to Your Project

1. Copy the `cef_artifacts` folder containing the compiled CEF artifacts into your Godot project root.
2. Delete the `cef_artifacts/cache` folder if you have previously used gdCEF.

> **Note:** Do not rename the folder or remove files inside. The Godot extension file (`.gdextension`) is included, so you don't need to create one.

### Custom Artifacts Folder Name

If you want to use a different name for the CEF artifacts folder:

**Option 1:** Modify `build.py` before compilation

- Find the line `CEF_ARTIFACTS_FOLDER_NAME = "cef_artifacts"`
- Change it to your desired name
- Rerun `build.py`

**Option 2:** Specify at runtime in your GDScript

```gdscript
$CEF.initialize({"artifacts": "res://your_custom_folder/", ... })
```

## Step 2: Set Up Your Scene

1. From the node selector, add a Godot `TextureRect` to your scene graph to hold your browser's texture.
2. From the node selector, look for the `GDCEF` node and add it to your scene.
   - If not found, the GDExtension file hasn't been loaded properly.

## Step 3: Create Your Browser

Create a GDScript and attach it to your GDCEF node:

```gdscript
extends GDCEF

func _ready():
    # Initialize CEF (required before creating browsers)
    initialize({})
    
    # Create a browser
    var browser = create_browser("https://github.com/Lecrapouille/gdCEF", $TextureRect, {})
    browser.set_name("my_browser")
```

### Browser Creation Parameters

- **URL**: The initial URL to load
- **TextureRect**: The Godot node that will display the browser content
- **Settings**: Optional dictionary for browser-specific settings (see [API documentation](API.md))

### Accessing Your Browser

Browsers are created as child nodes with default names `browser_0`, `browser_1`, etc. You can:

- Rename them: `browser.set_name("my_browser")`
- Find them: `$CEF.get_node("my_browser")` or `$CEF.get_node("browser_0")`

## Step 4: Handle Input Events

By default, the browser does not respond to mouse or keyboard inputs. You need to forward input events to the browser.

Check the [2D demo](../demos/2D/) and [3D demo](../demos/3D/) to learn how to:

- Forward mouse events (clicks, movements, scrolling)
- Forward keyboard events
- Handle focus

## Running Your Project

CEF can run directly from the Godot editor. You can also export your project for Linux, Windows, and macOS as usual.

> **Important:** The gdCEF module verifies the presence of both CEF artifacts and the secondary CEF process. If either is missing, your application will close with an error.

## Example Projects

For inspiration, check out:

- [Wattesigma](https://github.com/face-hh/wattesigma) - A full browser built with Godot
- The [demos](../demos/) included in this repository

## Next Steps

- Read the full [API documentation](API.md)
- Explore the [demos](../demos/README.md)
- Check the [FAQ](faq.md) for common issues
