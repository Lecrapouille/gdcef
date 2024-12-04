# CEF gdscript API

Two main classes:
- The CEF node `GDCef` managing browsers. You only need a single `GDCef` node for your project.
- The CEF browser nodes `GDBrowserView` displaying web documents (aka webviews). You should not create directly a `GDBrowserView` node manually in the scene graph. Use instead `GDCef.create_browser`. Browser are created as child nodes of the `GDCef` node (but Godot does not show them in the scene graph).

---

## API for `GDCef` node

`GDCef` is a class deriving from Godot's Node and interfaces with the core of the Chromium Embedded Framework. This class can create instances of `GDBrowserView` (displaying web documents) and store them as Godot child nodes (accessible via `get_node`).

### GDCef functions

| Godot function name | Arguments | Return | Comment |
|--------------------|-----------|---------|---------|
| `_init` | | | Dummy function. Use `initialize` instead. |
| `_process` | `dt`: float | void | Hidden function called automatically by Godot to process CEF internal messages (pump). `dt` is not used. |
| `create_browser` | `url`: String, `texture_rect`: TextureRect, `settings`: Dictionary | GDBrowserView | Creates a browser tab and stores its instance as child node. `url`: the page link. `texture_rect`: the container holding the texture. `settings`: optional settings applied to newly created browsers. |
| `get_error` | | String | Gives the reason of the failure of the latest GDCef function. |
| `get_version` | | String | Returns the full CEF version as string |
| `get_version_part` | `part`: int | int | Returns part of the CEF version as integer |
| `initialize` | `config`: Dictionary | bool | Replaces Godot _init() and passes optional [CEF configuration](#cef-configuration-initialize). Returns false in case of failure or double initialization, in this case you should halt the execution of your application. |
| `is_alive` | | bool | Returns if the `GDCef` is alive. |
| `shutdown` | | | Releases CEF memory and notifies sub CEF processes that the application is exiting. All browsers are destroyed. `GDCef` becomes inactive |

### GDCef signals

None.

### GDCef properties

None.

---

### CEF settings

Since Godot `_init` does not accept arguments, you must use the `initialize` function instead. This function takes a GDScript Dictionary to configure CEF behavior. Here are the available settings:

| Setting | Default Value | Description |
|---------|--------------|-------------|
| artifacts | CEF_ARTIFACTS_FOLDER | Path where CEF artifacts are stored. Required for CEF operation. Can be a local path, global path, or Godot path (starting with "res://"). Default value is set during compilation with build.py script. Referenced as `cef_folder_path` below. |
| exported_artifacts | application_real_path() | Path where CEF artifacts are stored when the Godot application is exported. Defines `cef_folder_path`. |
| incognito | false | When enabled, uses in-memory caches instead of disk storage. No data is persisted. |
| cache_path | cef_folder_path / "cache" | Directory for storing CEF caches. |
| root_cache_path | cef_folder_path / "cache" | Root directory for CEF cache storage. |
| browser_subprocess_path | cef_folder_path / SUBPROCESS_NAME | Path to the CEF subprocess executable. Name is determined during compilation. |
| log_file | cef_folder_path / "debug.log" | Path for CEF log files. |
| log_severity | "warning" | Log verbosity level. Options: "verbose", "info", "warning", "error", "fatal". |
| remote_allow_origin | "*" | Controls which origins can connect for debugging. "*" allows all. |
| remote_debugging_port | 7777 | Port for Chrome DevTools debugging. Access via http://localhost:7777. |
| exception_stack_size | 5 | Size of the exception stack. |
| locale | "en-US" | Browser language setting. |
| enable_media_stream | false | Controls access to camera and microphone. |

For additional settings, refer to [cef_types.h](../thirdparty/cef_binary/include/internal/cef_types.h).

## API for `GDBrowserView` nodes

Nodes are created by `GDCef.create_browser` and are automatically destroyed when `GDCef` is shut down. Since they inherit from `Node`, you can use Godot's scene graph functions to manage them, i.e. give them a name with `.name = "xxx".

### Browser functions

| Godot function name | Arguments | Return | Comment |
|--------------------|-----------|---------|---------|
| `_init` | | | Dummy function. Use `initialize` instead. |
| `allow_downloads` | `allow`: bool | | Enable or disable file downloads. |
| `close` | | | Close the browser and release its memory. |
| `copy` | | | Copy the current selection to clipboard. |
| `cut` | | | Cut the current selection to clipboard. |
| `download_file` | `url`: String | | Start downloading a file from the specified URL. The download folder is set with `set_download_folder` or by the configuration passed to `GDCef.create_browser`. |
| `execute_javascript` | `javascript`: string | | Execute custom JavaScript code in the browser window. This is a simple function to execute a string of JavaScript code. No value returned. |
| `get_error` | | String | Gives the reason of the failure of the latest GDBrowserView function. |
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
| `next_page` | | | Navigate to the next page if possible. |
| `paste` | | | Paste clipboard content at cursor position. |
| `previous_page` | | | Navigate to the previous page if possible. |
| `redo` | | | Redo the last undone edit action. |
| `request_html_content` | | | Request the current page HTML content. Result will be sent through the `on_html_content_requested` signal. |
| `resize` | `size`: Vector2 | | Resize the browser viewport to the specified width and height. |
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
| `set_mouse_wheel_horizontal` | `delta`: int | | Send a horizontal mouse wheel scroll event. |
| `set_mouse_wheel_vertical` | `delta`: int | | Send a vertical mouse wheel scroll event. |
| `set_muted` | `mute`: bool | bool | Set the audio mute state. Returns true if the audio was successfully muted. |
| `set_viewport` | `x`: float, `y`: float, `width`: float, `height`: float | | Set the viewport rectangle where the web content will be displayed. Values are in percent (0.0-1.0) of the surface dimensions. Default is x=0, y=0, width=1, height=1 (full surface). |
| `set_zoom_level` | `delta`: float | | Set the browser zoom level. |
| `stop_loading` | | | Stop loading the current page. |
| `undo` | | | Undo the last edit action. |

### Browser signals

| Signal name | Arguments | Description |
|-------------|-----------|-------------|
| `on_browser_paint` | `browser`: GDBrowserView | Emitted when the browser content has been painted to the texture. |
| `on_page_loaded` | `browser`: GDBrowserView | Emitted when a page has been successfully loaded. |
| `on_page_failed_loading` | `err_code`: int, `err_msg`: String, `browser`: GDBrowserView | Emitted when a page failed to load with the error code and message. |
| `on_download_updated` | `file`: String, `percentage`: int, `browser`: GDBrowserView | Emitted when a file download progress is updated. The path of the downloading file and the percentage of completion are given. |
| `on_html_content_requested` | `html`: String, `browser`: GDBrowserView | Emitted in response to `request_html_content()` with the page HTML content. |

### Browser properties

| Property name | Type | Description |
|---------------|------|-------------|
| `texture` | ImageTexture | Godot texture containing the page content for rendering in other Godot nodes. |
| `audio_stream` | AudioStreamGeneratorPlayback | Audio stream for playing audio content. |

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

For more information about available settings, see [cef_types.h](../thirdparty/cef_binary/include/internal/cef_types.h).

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