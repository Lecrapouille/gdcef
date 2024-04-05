# CEF gdscript API

Two classes:
- the CEF process `GDCef` making alive browsers.
- the browser `GDBrowserView` displaying web document.

## GDCef

`GDCef` is a class deriving from Godot's Node and interfacing the core of the
Chromium Embedded Framework. This class can create instances of `GDBrowserView`
(aka browser tabs) and store them as Godot child nodes (can be accessed from
`get_node`). You only need to have a single node of `GDCef` for your project.

| Godot function name | Arguments                                       | Return         | Comment                                                                                                                                                                                                 |
|---------------------|-------------------------------------------------|----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `_init`             |                                                 |                | Dummy function. Use instead `initialize`.                                                                                                                                                               |
| `initialize`        | `config`: godot::Dictionary                     | bool           | Replace Godot _init() and passing optional CEF configuration. Return false in case of failure or double initialisation.                                                                                 |
| `is_alive`          |                                                 | bool           | Return if the `GDCef` is alive.                                                                                                                                                                         |
| `_process`          | `dt`: float                                     | void           | Hidden function called automatically by Godot and call CEF internal pump messages. `dt` is not used.                                                                                                    |
| `create_browser`    | `url`: string, `texture_rect`: TextureRect, `settings`: godot::Dictionary | GDBrowserView* | Create a browser tab and store its instance as child node. `url`: the page link. `texture_rect`: the container holding the texture. `settings` optional settings for the created browser. |
| `shutdown`          |                                                 |                | Release CEF memory and sub CEF processes are notified that the application is exiting. All browsers are destroyed. `GDCef` is no more alive.                                                            |
| `get_error`         |                                                 | String         | Return the latest error.                                                                                                                                                                                |
| `get_version`       |                                                 | String         | Return the full CEF version as string.                                                                                                                                                                  |
| `get_version_part`  | `entry`: int                                    | int            | Return part of the CEF version as integer.                                                                                                                                                              |

Depending for `entry` concerning the `get_version_part`:
- 0: CEF_VERSION_MAJOR
- 1: CEF_VERSION_MINOR
- 2: CEF_VERSION_PATCH
- 3: CEF_COMMIT_NUMBER
- 4: CHROME_VERSION_MAJOR
- 5: CHROME_VERSION_MINOR
- 6: CHROME_VERSION_BUILD
- 7: CHROME_VERSION_PATCH

### CEF configuration: initialize()

Since Godot `_init` does not accept passing arguments, you have to use `initialize` function
instead. You also have to pass a Godot dictionary to configurate
the behavior for CEF. Default values are:
- `{"artifcats":CEF_ARTIFACTS_FOLDER}` Path where the build CEF artifcats are stored. They are
  needed to make CEF running and therefore your application. Fort this section, we will give the
  name `cef_folder_path` to the result. The default value is given during the compilation with
  the build.py script. You can specify either a local path or a global path or a Godot path
  (starting with `"res://"`).
- `{"exported_artifcats":application_real_path()}` Path where the build CEF artifcats are stored
  when the Godot application is exported. Artifcats are needed to make CEF running and therefore
  your application. It defines `cef_folder_path`.
- `{"incognito":false}` : In incognito mode cache directories not used and in-memory caches are
  used instead and no data is persisted to disk.
- `{"cache_path":cef_folder_path / "cache"}` : Folder path where to store CEF caches.
- `{"root_cache_path":cef_folder_path / "cache"}`
- `{"browser_subprocess_path":cef_folder_path / SUBPROCESS_NAME }` : canonical path to the CEF
  subprocess called during the `initialize()` function. The default name is determined during the
  compilation done with the build.py script.
- `{"log_file":cef_folder_path / "debug.log"}` : Where to store logs.
- `{log_severity":"warning"}` : Verbosity control of logs. Choose between `"verbose"`, `"info"`,
  `"warning"`, `"error"`, `"fatal"`.
- `{"remote_debugging_port":7777}` : the port for debbuging CEF.
- `{"exception_stack_size":5}`
- `{"locale":"en-US"}` : Select your language.
- `{"enable_media_stream", false}` : allow CEF to access to your camera and microphones.

See [cef_types.h](../thirdparty/cef_binary/include/internal/cef_types.h) for more information about settings.

### Browser configuration: create_browser()

You can modify browser configuration when creating one with `create_browser`. Default values are:
- `{"frame_rate":30}`
- `{"javascript":true}`
- `{"javascript_close_windows":false}`
- `{"javascript_access_clipboard":false}`
- `{"javascript_dom_paste":false}`
- `{"image_loading":true}`
- `{"databases":true}`
- `{"webgl":true}`

See [cef_types.h](../thirdparty/cef_binary/include/internal/cef_types.h) for more information about settings.

## GDBrowserView

Class wrapping the `CefBrowser` class and export methods for Godot script.
This class is instanciate by `GDCef` by `create_browser`.

### GDBrowserView Functions

| Godot function name     | Arguments                                                   | Return            | Comment                                                                                                                                                                                                                                     |
|-------------------------|-------------------------------------------------------------|-------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `_init`                 |                                                             |                   | Dummy function. Use instead `initialize`. |
| `close`                 |                                                             |                   | Close the browser. Memory is released.                                                                                                                                                                                                      |
| `id`                    |                                                             | int               | Return the unique browser identifier.                                                                                                                                                                                                       |
| `is_valid`              |                                                             | bool              | Return `true` if this object is currently valid.                                                                                                                                                                                            |
| `get_texture`           |                                                             | Ref ImageTexture  | Return the Godot texture holding the page content to other Godot element that needs it for the rendering.                                                                                                                                   |
| `set_zoom_level`        | `delta`: float                                              |                   | Set the render zoom level.                                                                                                                                                                                                                  |
| `load_data_uri`         | `html`: string, `mime_type`: string                         |                   | Load the given html content page                                                                                                                                                                                                                     |
| `load_url`              | `url`: string                                               |                   | Load the given web page                                                                                                                                                                                                                     |
| `is_loaded`             |                                                             | bool              | Return `true` if a document has been loaded in the browser.                                                                                                                                                                                 |
| `get_url`               |                                                             | string            | Get the current url of the browser.                                                                                                                                                                                                         |
| `stop_loading`          |                                                             |                   | Stop loading the page.                                                                                                                                                                                                                      |
| `execute_javascript`    | `javascript` : string                                       |                   | Execute custom javascript in the browser window.                                                                                                                                                                                            |
| `has_previous_page`     |                                                             | bool              | Return true if the browser can navigate to the previous page.                                                                                                                                                                               |
| `has_next_page`         |                                                             | bool              | Return true if the browser can navigate to the next page.                                                                                                                                                                                   |
| `previous_page`         |                                                             |                   | Navigate to the previous page if possible.                                                                                                                                                                                                  |
| `next_page`             |                                                             |                   | Navigate to the next page if possible.                                                                                                                                                                                                      |
| `resize`                | `(width, height)`: Vector2                                  |                   | Resize the page (width, height).                                                                                                                                                                                                                   |
| `set_viewport`          | `x`: float, `y`: float, `width`: float, `height`: float     |                   | the rectangle on the surface where to display the web document. Values are in percent of the dimension on the surface. If this function is not called default values are: x = y = 0 and w = h = 1 meaning the whole surface will be mapped. |
| `set_key_pressed`       | `key`: int, `pressed`: bool, `shift`: bool, `alt`: bool, `ctrl`: bool |         | Set the new keyboard state (char typed ...)                                                                                                                                                                                                 |
| `set_mouse_moved`       | `x`: int, `y`: int                                          |                   | Set the new mouse position.                                                                                                                                                                                                                 |
| `set_mouse_left_click`  |                                                             |                   | Down then up on Left button                                                                                                                                                                                                                 |
| `set_mouse_right_click` |                                                             |                   | Down then up on Right button.                                                                                                                                                                                                               |
| `set_mouse_middle_click`|                                                             |                   | Down then up on middle button.                                                                                                                                                                                                              |
| `set_mouse_left_down`   |                                                             |                   | Left Mouse button down.                                                                                                                                                                                                                     |
| `set_mouse_left_up`     |                                                             |                   | Left Mouse button up.                                                                                                                                                                                                                       |
| `set_mouse_right_down`  |                                                             |                   | Right Mouse button down.                                                                                                                                                                                                                    |
| `set_mouse_right_up`    |                                                             |                   | Right Mouse button up.                                                                                                                                                                                                                      |
| `set_mouse_middle_down` |                                                             |                   | Middle Mouse button down.                                                                                                                                                                                                                   |
| `set_mouse_middle_up`   |                                                             |                   | Middle Mouse button up.                                                                                                                                                                                                                     |
| `set_mouse_wheel_vertical`  | `delta`: int                                            |                   | Vertical Mouse Wheel scrolling.                                                                                                                                                                                                             |
| `set_mouse_wheel_horizontal`| `delta`: int                                            |                   | Horizontal Mouse Wheel scrolling.                                                                                                                                                                                                           |
| `set_muted`             | `mute`: bool                                                | bool              | Set the audio mute state (true: to mute the sound, false: to unmute). Return true if the sound has been muted.                                                                                                                              |
| `is_muted`              |                                                             | bool              | Get the current mute state : returns true if the sound is muted.                                                                                                                                                                            |
| `get_error`             |                                                             | String            | Return the latest error.                                                                                                                                                                                                                    |

### GDBrowserView Callbacks

| Godot callback name   | Arguments                                                   | Return            | Comment                                                                                              |
|-------------------------|-------------------------------------------------------------|-------------------|------------------------------------------------------------------------------------------------------|
| `on_browser_paint`      | `node`: GDBrowserView                                       |                   | Triggered when a page has been painted in the texture.                                               |
| `on_page_loaded`         | `node`: GDBrowserView                                       |                   | Triggered when a page has been successfuly loaded.                                                   |
| `on_page_failed_loading` | `aborted`: bool, `err_msg`: string, `node`: GDBrowserView   |                   | Triggered when a page has been failed loading: aborted by the user or failure                        |
