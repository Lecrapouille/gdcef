# CEF gdscript API

Two main classes:

- The CEF node `gdCEF` managing browsers. You only need a single `gdCEF` node for your project.
- The CEF browser nodes `GdBrowserView` displaying web documents (aka webviews). You should not create directly a `GdBrowserView` node manually in the scene graph. Use instead `gdCEF.create_browser`. Browser are created as child nodes of the `gdCEF` node (but Godot does not show them in the scene graph).

---

## API for `gdCEF` node

`gdCEF` is a class deriving from Godot's Node and interfaces with the core of the Chromium Embedded Framework. This class can create instances of `GdBrowserView` (displaying web documents) and store them as Godot child nodes (accessible via `get_node`).

### gdCEF functions

| Godot function name | Arguments | Return | Comment |
|--------------------|-----------|---------|---------|
| `_init` | | | Dummy function. Use `initialize` instead. |
| `_process` | `dt`: float | void | Hidden function called automatically by Godot to process CEF internal messages (pump). `dt` is not used. |
| `create_browser` | `url`: String, `texture_rect`: TextureRect, `settings`: Dictionary | GdBrowserView | Creates a browser tab and stores its instance as child node. `url`: the page link. `texture_rect`: the container holding the texture. `settings`: optional settings applied to newly created browsers. |
| `get_error` | | String | Gives the reason of the failure of the latest gdCEF function. |
| `get_full_version` | | String | Returns the full CEF version as string |
| `get_version_part` | `part`: int | int | Returns part of the CEF version as integer |
| `initialize` | `config`: Dictionary | bool | Replaces Godot _init() and passes optional [CEF configuration](#cef-settings). Returns false in case of failure or double initialization, in this case you should halt the execution of your application. |
| `is_alive` | | bool | Returns if the `gdCEF` is alive. |
| `log_info` | `message`: String | void | Write an info message to CEF logs from GDScript. |
| `log_warning` | `message`: String | void | Write a warning message to CEF logs from GDScript. |
| `log_error` | `message`: String | void | Write an error message to CEF logs from GDScript. |
| `log_fatal` | `message`: String | void | Write a fatal message to CEF logs from GDScript. Logged with the error severity: it does not terminate the application. |
| `shutdown` | | | Releases CEF memory and notifies sub CEF processes that the application is exiting. All browsers are destroyed. `gdCEF` becomes inactive |

### gdCEF signals

None.

### gdCEF properties

None.

---

### CEF settings

Since Godot `_init` does not accept arguments, you must use the `initialize` function instead. This function takes a GDScript Dictionary to configure CEF behavior. Here are the available settings:

| Setting | Default Value | Description |
|---------|--------------|-------------|
| incognito | false | When enabled, uses in-memory caches instead of disk storage. No data is persisted. |
| cache_path | cef_folder_path / "cache" | Directory for storing CEF caches. |
| root_cache_path | cef_folder_path / "cache" | Root directory for CEF cache storage. |
| browser_subprocess_path | cef_folder_path / SUBPROCESS_NAME | Path to the CEF subprocess executable. Name is determined during compilation. |
| log_file | cef_folder_path / "debug.log" | Path for CEF log files. |
| log_severity | "warning" | Log verbosity level. Options: "verbose", "info", "warning", "error", "fatal". |
| remote_allow_origin | "*" | Controls which origins can connect for debugging. "*" allows all. |
| remote_debugging_port | 7777 | Port for Chrome DevTools debugging. Access via [http://localhost:7777](http://localhost:7777). |
| exception_stack_size | 5 | Size of the exception stack. |
| locale | "en-US" | Browser language setting. |
| enable_media_stream | false | Controls access to camera and microphone. |
| use_gpu | true | Use the system GPU for Chromium rendering via ANGLE. When `false`, falls back to SwiftShader software rendering (CPU-only, slower but more compatible). See [GPU rendering](#gpu-rendering). |

For additional settings, refer to [cef_types.h](../thirdparty/cef_binary/include/internal/cef_types.h).

#### GPU rendering

gdCEF renders web pages off-screen (OSR). Chromium still needs an internal graphics backend for rasterization, compositing, and WebGL. The `use_gpu` setting selects that backend at startup by appending command-line switches in `OnBeforeCommandLineProcessing`:

| `use_gpu` | Chromium switches | Effect |
|-----------|-------------------|--------|
| `true` (default) | `use-angle=default`, `use-gl=angle` | Native GPU via ANGLE. Much lower input-to-photon latency, faster first paint, and WebGL runs on the GPU. Recommended for interactive UIs and touch kiosks. |
| `false` | `use-angle=swiftshader`, `use-gl=angle` | SwiftShader software renderer. All rasterization and WebGL emulation run on the CPU. Slower, but avoids GPU-related crashes or black screens on some systems. |

This setting must be passed to `initialize()` before CEF starts; it cannot be changed later.

```gdscript
# Default: GPU enabled
$CEF.initialize({"locale": "en-US"})

# Explicit GPU (same as default)
$CEF.initialize({"use_gpu": true, "locale": "en-US"})

# Software fallback for problematic hardware
$CEF.initialize({"use_gpu": false, "locale": "en-US"})
```

If you see a black browser texture or instability after enabling the GPU, set `use_gpu` to `false`. For background on GPU rendering in OSR mode, see the [CEF forum thread](https://magpcss.org/ceforum/viewtopic.php?f=17&t=18970).

## API for `GdBrowserView` nodes

Nodes are created by `gdCEF.create_browser` and are automatically destroyed when `gdCEF` is shut down. Since they inherit from `Node`, you can use Godot's scene graph functions to manage them, i.e. give them a name with `.name = "xxx".

### Browser functions

| Godot function name | Arguments | Return | Comment |
|--------------------|-----------|---------|---------|
| `_init` | | | Dummy function. Use `initialize` instead. |
| `allow_downloads` | `allow`: bool | | Enable or disable file downloads. |
| `close` | | | Close the browser and release its memory. |
| `copy` | | | Copy the current selection to clipboard. |
| `cut` | | | Cut the current selection to clipboard. |
| `download_file` | `url`: String | | Start downloading a file from the specified URL. The download folder is set with `set_download_folder` or by the configuration passed to `gdCEF.create_browser`. |
| `execute_javascript` | `javascript`: string | | Execute custom JavaScript code in the browser window. This is a simple function to execute a string of JavaScript code. No value returned. |
| `get_error` | | String | Gives the reason of the failure of the latest GdBrowserView function. |
| `get_pixel_color` | `x`: int, `y`: int | Color | Get the color of the pixel at the specified coordinates. |
| `get_texture` | | Ref ImageTexture | Return the Godot texture containing the page content for rendering in other Godot nodes. |
| `get_title` | | string | Get the current web document title. |
| `get_url` | | string | Get the current URL of the browser. |
| `has_next_page` | | bool | Return `true` if the browser can navigate to the next page. |
| `has_previous_page` | | bool | Return `true` if the browser can navigate to the previous page. |
| `id` | | int | Return the unique browser identifier. |
| `is_loaded` | | bool | Return `true` if a document has been loaded in the browser. |
| `is_muted` | | bool | Get the current audio mute state. Returns true if audio is muted. |
| `is_valid` | | bool | Return `true` if this browser instance is currently valid. |
| `load_data_uri` | `html`: string, `mime_type`: string | | Load HTML content directly with the specified MIME type. |
| `load_url` | `url`: string | | Load the specified web page URL. |
| `log_info` | `message`: String | void | Write an info message to CEF logs from GDScript. |
| `log_warning` | `message`: String | void | Write a warning message to CEF logs from GDScript. |
| `log_error` | `message`: String | void | Write an error message to CEF logs from GDScript. |
| `log_fatal` | `message`: String | void | Write a fatal message to CEF logs from GDScript. Logged with the error severity: it does not terminate the application. |
| `next_page` | | | Navigate to the next page if possible. |
| `paste` | | | Paste clipboard content at cursor position. |
| `previous_page` | | | Navigate to the previous page if possible. |
| `redo` | | | Redo the last undone edit action. |
| `register_method` | `callable`: Callable | bool | Register a GDScript method to be callable from JavaScript. Returns true if registration was successful. |
| `reload` | | bool | Reload the current page. Returns true if reload was initiated. |
| `request_html_content` | | | Request the current page HTML content. Result will be sent through the `on_html_content_requested` signal. |
| `save_page` | `path`: String | | Save the current page HTML content to a file. Supports Godot paths (`res://`, `user://`) and absolute paths. Result will be sent through the `on_page_saved` signal. |
| `save_page_as_pdf` | `path`: String | | Save the current page as a PDF file with all rendered content (images, CSS, etc.). Supports Godot paths. Result will be sent through the `on_pdf_saved` signal. |
| `resize` | `size`: Vector2 | | Resize the browser viewport to the specified width and height. |
| `emit_js` | `event_name`: String, `data`: Variant | bool | Send data to JavaScript. Returns true if the data was successfully sent. |
| `set_download_folder` | `path`: String | | Set the folder where downloaded files will be saved. Accepts absolute paths or Godot paths (`res://`, `user://`). |
| `set_key_pressed` | `key`: int, `pressed`: bool, `shift`: bool, `alt`: bool, `ctrl`: bool | | Send a keyboard event to the browser. |
| `set_mouse_left_click` | | | Send a left mouse button click event (down then up). |
| `set_mouse_left_down` | | | Send a left mouse button down event. |
| `set_mouse_left_up` | | | Send a left mouse button up event. |
| `set_mouse_middle_click` | | | Send a middle mouse button click event (down then up). |
| `set_mouse_middle_down` | | | Send a middle mouse button down event. |
| `set_mouse_middle_up` | | | Send a middle mouse button up event. |
| `set_mouse_moved` | `x`: int, `y`: int | | Send a mouse move event to the browser. |
| `set_mouse_right_click` | | | Send a right mouse button click event (down then up). |
| `set_mouse_right_down` | | | Send a right mouse button down event. |
| `set_mouse_right_up` | | | Send a right mouse button up event. |
| `set_mouse_wheel_horizontal` | `delta`: int, `shift`: bool, `ctrl`: bool, `alt`: bool | | Send a horizontal mouse wheel scroll event with optional keyboard modifiers. |
| `set_mouse_wheel_vertical` | `delta`: int, `shift`: bool, `ctrl`: bool, `alt`: bool | | Send a vertical mouse wheel scroll event with optional keyboard modifiers (e.g., Ctrl+scroll for zoom on Google Maps). |
| `set_muted` | `mute`: bool | bool | Set the audio mute state. Returns true if the audio was successfully muted. |
| `set_viewport` | `x`: float, `y`: float, `width`: float, `height`: float | | Set the viewport rectangle where the web content will be displayed. Values are in percent (0.0-1.0) of the surface dimensions. Default is x=0, y=0, width=1, height=1 (full surface). |
| `set_zoom_level` | `delta`: float | | Set the browser zoom level. |
| `set_audio_stream` | `audio`: AudioStreamGeneratorPlayback | | Set the audio stream playback for browser audio. |
| `get_audio_stream` | | AudioStreamGeneratorPlayback | Get the current audio stream playback. |
| `stop_loading` | | | Stop loading the current page. |
| `undo` | | | Undo the last edit action. |
| `add_ad_block_pattern` | `pattern`: String | bool | Add a rule in EasyList format: `\|\|domain.com^` and `domain.com` block a domain and its subdomains, `/path/pattern` and `keyword` are searched inside the whole URL, a `@@` prefix whitelists instead of blocking. Returns true if the rule was understood. |
| `load_ad_block_filter_list` | `filepath`: String | int | Add all the rules of an EasyList compatible filter list file (one rule per line, lines starting with `!` or `[` are comments). Returns the number of rules added. |
| `clear_ad_block_rules` | | void | Remove all the rules, including the default ones, to start from an empty filter list. |
| `get_ad_block_stats` | | String | Human readable description of the number of loaded rules. |
| `enable_ad_block` | `enable`: bool | void | Enable or disable the ad blocker. When enabled, common ad patterns are blocked by default. |
| `is_ad_block_enabled` | | bool | Check if the ad blocker is currently enabled. |

### Browser signals

| Signal name | Arguments | Description |
|-------------|-----------|-------------|
| `on_browser_paint` | `browser`: GdBrowserView | Emitted when the browser content has been painted to the texture. |
| `on_page_loaded` | `browser`: GdBrowserView | Emitted when a page has been loaded. |
| `on_page_failed_loading` | `err_code`: int, `err_msg`: String, `browser`: GdBrowserView | Emitted when a page failed to load with the error code and message. |
| `on_download_updated` | `file`: String, `percentage`: int, `browser`: GdBrowserView | Emitted when a file download progress is updated. The path of the downloading file and the percentage of completion are given. |
| `on_html_content_requested` | `html`: String, `browser`: GdBrowserView | Emitted in response to `request_html_content()` with the page HTML content. |
| `on_page_saved` | `path`: String, `success`: bool, `browser`: GdBrowserView | Emitted in response to `save_page()` with the file path and success status. |
| `on_pdf_saved` | `path`: String, `success`: bool, `browser`: GdBrowserView | Emitted in response to `save_page_as_pdf()` with the file path and success status. |
| `on_drag_enter` | `drag_info`: Dictionary, `browser`: GdBrowserView | Emitted when content dragged from outside the page enters the browser. The dictionary holds the `type` of the content (`file`, `link`, `fragment` or `unknown`) and the matching data. |
| `on_draggable_regions_changed` | `regions`: Array, `browser`: GdBrowserView | Emitted when the page declares regions the window can be dragged by (CSS `-webkit-app-region`). |
| `on_start_dragging` | `drag_info`: Dictionary, `browser`: GdBrowserView | Emitted when the user starts dragging content inside the page. The dictionary holds the `type` of the content, the matching data and the `x`, `y` start position. |
| `on_update_drag_cursor` | `operation`: int (`0` when the content cannot be dropped, else a mask of drag operations), `browser`: GdBrowserView | Emitted while dragging, each time the drop target under the mouse changes. gdCEF already displays the hand or the forbidden cursor by itself, so connecting this signal is only needed to draw your own feedback. |
| `on_cursor_changed` | `cursor_shape`: int (`DisplayServer.CursorShape`), `browser`: GdBrowserView | Emitted when the page wants to change the mouse cursor (link hover, text selection, resize handles, etc.). The `Control` passed to `create_browser()` (e.g. `TextureRect`) automatically gets its `mouse_default_cursor_shape` updated by gdCEF itself, so you usually don't need to connect this signal at all. It is only provided in case your application wants to react to cursor changes itself (e.g. custom cursor rendering). Note: a plain `Input.set_default_cursor_shape()` call from your own code would not be enough on its own, since a `Control` covering the browser area always wins over it on every mouse motion event. |

### Browser properties

| Property name | Type | Description |
|---------------|------|-------------|
| `audio_stream` | AudioStreamGeneratorPlayback | Audio stream for playing audio content. |

The browser texture is not a property: it is owned and resized by the browser
itself, use `get_texture()` to read it.

### Browser settings

Browser settings are passed as a GDScript Dictionary to `create_browser`. Here are the available settings:

| Parameter | Type | Default Value | Description |
|-----------|------|---------------|-------------|
| frame_rate | int | 30 | Browser rendering refresh rate in frames per second |
| javascript | bool | true | Enable or disable JavaScript execution |
| javascript_close_windows | bool | false | Allow JavaScript to close windows |
| javascript_access_clipboard | bool | false | Allow JavaScript to access clipboard |
| javascript_dom_paste | bool | false | Allow JavaScript to perform DOM paste operations |
| image_loading | bool | true | Enable or disable image loading |
| databases | bool | true | Enable or disable web database support |
| webgl | bool | true | Enable or disable WebGL support |
| enable_ad_block | bool | true | Enable or disable the ad blocker |
| ad_block_patterns | Array[String] | [] | Additional regex patterns for blocking ads. Invalid patterns will be ignored. |
| allow_downloads | bool | true | Allow the browser for downloading data. |
| download_folder | string | "user://" | Desired location holding downloaded files. |
| user_agent | string | "" | Custom user agent string to use for the browser. |
| user_gesture_required | bool | true | false: Autoplay policy that does not require any user gesture. |

### CEF version

Part value for extraction CEF version with `get_version_part`:

| Part | Description |
|-------|-------------|
| 0 | CEF_VERSION_MAJOR |
| 1 | CEF_VERSION_MINOR |
| 2 | CEF_VERSION_PATCH |
| 3 | CEF_COMMIT_NUMBER |
| 4 | CHROME_VERSION_MAJOR |
| 5 | CHROME_VERSION_MINOR |
| 6 | CHROME_VERSION_BUILD |
| 7 | CHROME_VERSION_PATCH |

## JavaScript and GDScript Communication

gdCEF provides bidirectional communication between JavaScript executed in the browser and GDScript code in Godot. This communication is essential for creating interactive web interfaces that can exchange data with your game.

### Binding JavaScript to GDScript

Here's how to make your JavaScript code call GDScript functions:

1. **Register a GDScript method** with the browser:

```gdscript
# In your GDScript code
func my_function_for_js(data: Variant):
    print("Data received from JavaScript:", data)
    # Process data...
```

Better to register when the page has been loaded:

```gdscript
func _on_page_loaded(browser):
    browser.register_method(Callable(self, "my_function_for_js"))

browser.connect("on_page_loaded", _on_page_loaded)
```

2. **Call the GDScript method from JavaScript**:

```javascript
// In your JavaScript code
window.godotMethods.my_function_for_js("Data to send to Godot");

// You can send different data types
window.godotMethods.my_function_for_js(42);                  // Number
window.godotMethods.my_function_for_js(true);                // Boolean
window.godotMethods.my_function_for_js({key: "value"});      // Object
window.godotMethods.my_function_for_js([1, 2, 3]);           // Array
```

### Binding GDScript to JavaScript

To make your GDScript code send data to JavaScript:

1. **Set up an event listener in JavaScript**:

```javascript
// In your JavaScript code
window.godotEvents.on('event_name', function(data) {
    console.log('Data received from Godot:', data);
    // Process data...
});
```

2. **Send data from GDScript**:

```gdscript
# In your GDScript code
browser.emit_js("event_name", "Data to send to JavaScript")

# You can send different data types
browser.emit_js("event_name", 42)                   // Number
browser.emit_js("event_name", true)                 // Boolean
browser.emit_js("event_name", {"key": "value"})     // Dictionary
browser.emit_js("event_name", [1, 2, 3])            // Array
browser.emit_js("event_name", PackedByteArray([...])) # Binary data
```

CheatSheet Example:

See [this example](../tests/unit-test-js-gdscript/)

### Example of User agent

"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:121.0) Gecko/20100101 Firefox/121.0"

You can see the value, if you debug CEF (see [the FAQ section](../README.md)) and click on the "Network" menu. You will it in a GET method.
