# ==============================================================================
# Demo based on the initial asset https://godotengine.org/asset-library/asset/127
# Basic application showing how to use CEF inside Godot with a 3D scene and mouse
# and keyboard events.
# ==============================================================================

extends Control

# URL
const DEFAULT_PAGE = "user://default_page.html"
const SAVED_PAGE = "user://saved_page.html"
const SAVED_PDF = "user://saved_page.pdf"
const DRAG_DROP_PAGE = "user://dragdrop_page.html"
const HOME_PAGE = "https://github.com/Lecrapouille/gdcef"
const RADIO_PAGE = "https://streaming.radiostreamlive.com/radiorockon_devices"

# The current browser as Godot node
@onready var current_browser = null
# Memorize if the mouse was pressed
@onready var mouse_pressed: bool = false

# ==============================================================================
# Create the home page.
# ==============================================================================
func create_default_page():
	var file = FileAccess.open(DEFAULT_PAGE, FileAccess.WRITE)
	file.store_string("<html><body bgcolor=\"white\"><h2>Welcome to gdCEF !</h2><p>This a generated page.</p></body></html>")
	file.close()
	pass

# ==============================================================================
# Create the drag and drop test page (HTML5 Drag and Drop API).
# Based on: https://www.w3schools.com/html/html5_draganddrop.asp
# ==============================================================================
func create_dragdrop_page():
	var html = """<!DOCTYPE HTML>
<html>
<head>
<style>
body {
    font-family: Arial, sans-serif;
    padding: 20px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    margin: 0;
}
h1 {
    color: white;
    text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
}
.container {
    display: flex;
    gap: 20px;
    flex-wrap: wrap;
}
.drop-zone {
    width: 200px;
    height: 200px;
    border: 3px dashed rgba(255,255,255,0.5);
    border-radius: 15px;
    background: rgba(255,255,255,0.1);
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.3s ease;
}
.drop-zone:hover {
    border-color: white;
    background: rgba(255,255,255,0.2);
}
.drop-zone.drag-over {
    border-color: #4CAF50;
    background: rgba(76, 175, 80, 0.3);
    transform: scale(1.05);
}
.draggable {
    width: 80px;
    height: 80px;
    border-radius: 10px;
    cursor: grab;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 40px;
    box-shadow: 0 4px 15px rgba(0,0,0,0.3);
    transition: transform 0.2s ease;
}
.draggable:hover {
    transform: scale(1.1);
}
.draggable:active {
    cursor: grabbing;
}
#drag1 { background: linear-gradient(45deg, #FF6B6B, #FF8E53); }
#drag2 { background: linear-gradient(45deg, #4ECDC4, #44A08D); }
#drag3 { background: linear-gradient(45deg, #A8E6CF, #88D8B0); }
.info {
    color: white;
    margin-top: 20px;
    padding: 15px;
    background: rgba(0,0,0,0.2);
    border-radius: 10px;
}
</style>
<script>
function dragstartHandler(ev) {
    ev.dataTransfer.setData("text", ev.target.id);
    ev.target.style.opacity = '0.5';
}

function dragendHandler(ev) {
    ev.target.style.opacity = '1';
}

function dragoverHandler(ev) {
    ev.preventDefault();
    ev.currentTarget.classList.add('drag-over');
}

function dragleaveHandler(ev) {
    ev.currentTarget.classList.remove('drag-over');
}

function dropHandler(ev) {
    ev.preventDefault();
    ev.currentTarget.classList.remove('drag-over');
    const data = ev.dataTransfer.getData("text");
    const draggedElement = document.getElementById(data);
    if (draggedElement) {
        ev.currentTarget.appendChild(draggedElement);
    }
}
</script>
</head>
<body>

<h1>HTML5 Drag and Drop Test</h1>
<p style="color: white;">Drag the emoji boxes into the drop zones!</p>

<div class="container">
    <div id="zone1" class="drop-zone"
         ondrop="dropHandler(event)"
         ondragover="dragoverHandler(event)"
         ondragleave="dragleaveHandler(event)">
        <div id="drag1" class="draggable" draggable="true"
             ondragstart="dragstartHandler(event)"
             ondragend="dragendHandler(event)">Cible</div>
    </div>

    <div id="zone2" class="drop-zone"
         ondrop="dropHandler(event)"
         ondragover="dragoverHandler(event)"
         ondragleave="dragleaveHandler(event)">
        <div id="drag2" class="draggable" draggable="true"
             ondragstart="dragstartHandler(event)"
             ondragend="dragendHandler(event)">Fusee</div>
    </div>

    <div id="zone3" class="drop-zone"
         ondrop="dropHandler(event)"
         ondragover="dragoverHandler(event)"
         ondragleave="dragleaveHandler(event)">
        <div id="drag3" class="draggable" draggable="true"
             ondragstart="dragstartHandler(event)"
             ondragend="dragendHandler(event)">Etoile</div>
    </div>

    <div id="zone4" class="drop-zone"
         ondrop="dropHandler(event)"
         ondragover="dragoverHandler(event)"
         ondragleave="dragleaveHandler(event)">
    </div>
</div>

<div class="info">
    <strong>Instructions:</strong><br>
    1. Click and hold on an emoji box<br>
    2. Drag it to another drop zone<br>
    3. Release to drop it<br><br>
    <em>This tests the HTML5 Drag and Drop API in gdCEF!</em>
</div>

</body>
</html>"""
	var file = FileAccess.open(DRAG_DROP_PAGE, FileAccess.WRITE)
	file.store_string(html)
	file.close()
	pass

# ==============================================================================
# Callback when page has been saved to file (HTML).
# ==============================================================================
func _on_page_saved(path, success, browser):
	if success:
		$AcceptDialog.title = "Page Saved (HTML)"
		$AcceptDialog.dialog_text = "HTML saved successfully at:\n" + path
	else:
		$AcceptDialog.title = "Save Failed"
		$AcceptDialog.dialog_text = "Failed to save HTML to:\n" + path
	$AcceptDialog.popup_centered(Vector2(0, 0))
	$AcceptDialog.show()
	pass

# ==============================================================================
# Callback when page has been saved as PDF (with images, CSS, etc.).
# ==============================================================================
func _on_pdf_saved(path, success, browser):
	if success:
		$AcceptDialog.title = "Page Saved (PDF)"
		$AcceptDialog.dialog_text = "PDF saved successfully at:\n" + path + "\n\nIncludes all images and styles!"
	else:
		$AcceptDialog.title = "PDF Save Failed"
		$AcceptDialog.dialog_text = "Failed to save PDF to:\n" + path
	$AcceptDialog.popup_centered(Vector2(0, 0))
	$AcceptDialog.show()
	pass

# ==============================================================================
# Legacy callback for raw HTML content (kept for compatibility).
# ==============================================================================
func _on_html_content_requested(html, browser):
	print("HTML content received: " + str(html.length()) + " characters")
	pass

# ==============================================================================
# Callback when a download file is updated
# ==============================================================================
func _on_download_updated(file, percentage, browser):
	$AcceptDialog.title = "Downloading!"
	$AcceptDialog.dialog_text = file + " " + str(percentage) + " %"
	$AcceptDialog.popup_centered(Vector2(0, 0))
	$AcceptDialog.show()

# ==============================================================================
# Callback when a page has ended to load with success (200): we print a message
# ==============================================================================
func _on_page_loaded(browser):
	var L = $Panel/VBox/TopBar/BrowserList
	var url = browser.get_url()
	L.set_item_text(L.get_selected_id(), url)
	$Panel/VBox/BottomBar/Info.set_text(url)
	# Update URL bar
	$Panel/VBox/TopBar/URLContainer/TextEdit.text = url
	print("Browser named '" + browser.name + "' inserted on list at index " + str(L.get_selected_id()) + ": " + url)
	# Logging from browser instance
	browser.log_warning("This is an example warning")
	pass

# ==============================================================================
# Callback when a page has ended to load with failure.
# Display an error message in a generated HTML page, using data URI.
# List of error are defined in the following file:
# thirdparty/cef_binary/include/base/internal/cef_net_error_list.h
# ==============================================================================
func _on_page_failed_loading(err_code, err_msg, browser):
	var html = "<html><body bgcolor=\"white\">" \
		+"<h2>Failed to load URL " + browser.get_url() + "!</h2>" \
		+"<p>Error code: " + str(err_code) + "</p>" \
		+"<p>Error message: " + err_msg + "!</p>" \
		+"</body></html>"
	browser.load_data_uri(html, "text/html")
	pass

# ==============================================================================
# Create a new browser and return it or return null if failed.
# ==============================================================================
func create_browser(url):
	# Wait one frame for the texture rect to get its size
	await get_tree().process_frame

	# See API.md for more details. Possible browser configuration is:
	# {
	#   "frame_rate": 30,
	#   "javascript": true,
	#   "javascript_close_windows": false,
	#   "javascript_access_clipboard": false,
	#   "javascript_dom_paste": false,
	#   "image_loading": true,
	#   "databases": true,
	#   "webgl": true,
	#   "allow_downloads": false,
	#   "download_folder": "res://",
	#   "user_gesture_required": true,
	# }
	var browser = $CEF.create_browser(url, $Panel/VBox/TextureRect,
		{
			"javascript": true,
			"webgl": true,
			"user_gesture_required": true
		})
	if browser == null:
		$Panel/VBox/BottomBar/Info.set_text($CEF.get_error())
		return null

	# Loading callbacks
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	browser.connect("on_download_updated", _on_download_updated)
	browser.connect("on_page_saved", _on_page_saved)
	browser.connect("on_pdf_saved", _on_pdf_saved)
	browser.connect("on_html_content_requested", _on_html_content_requested)

	# Add the URL to the list
	var browser_list = $Panel/VBox/TopBar/BrowserList
	browser_list.add_item(url)
	browser_list.select(browser_list.get_item_count() - 1)
	print("Browser named '" + browser.name + "' created with URL " + url)
	return browser

# ==============================================================================
# Search the desired by its name. Return the browser as Godot node or null if
# not found.
# ==============================================================================
func get_browser(name):
	if not $CEF.is_alive():
		return null
	var browser = $CEF.get_node(name)
	if browser == null:
		$Panel/VBox/BottomBar/Info.set_text("Unknown browser with name '" + name + "'")
		return null
	return browser

####
#### Top menu
####

# ==============================================================================
# Create a new browser node. Note: Godot does not show children nodes so you
# will not see created browsers as sub nodes.
# ==============================================================================
func _on_Add_pressed():
	var browser = await create_browser("file://" + ProjectSettings.globalize_path(DEFAULT_PAGE))
	if browser != null:
		current_browser = browser
	pass

# ==============================================================================
# Home button pressed: load a local HTML document.
# ==============================================================================
func _on_Home_pressed():
	if current_browser != null:
		current_browser.load_url(HOME_PAGE)
	pass

# ==============================================================================
# Save button pressed: save current page as HTML file.
# ==============================================================================
func _on_Save_pressed():
	if current_browser != null:
		current_browser.save_page(SAVED_PAGE)
	pass

# ==============================================================================
# Save PDF button pressed: save current page as PDF with all resources.
# ==============================================================================
func _on_SavePdf_pressed():
	if current_browser != null:
		current_browser.save_page_as_pdf(SAVED_PDF)
	pass

# ==============================================================================
# Go to the URL given by the text edit widget.
# ==============================================================================
func _on_go_pressed():
	if current_browser != null:
		current_browser.load_url($Panel/VBox/TopBar/URLContainer/TextEdit.text)
	pass

# ==============================================================================
# URL submitted via Enter key in the text edit.
# ==============================================================================
func _on_url_submitted(new_text):
	if current_browser != null:
		current_browser.load_url(new_text)
	pass

# ==============================================================================
# Reload the current page
# ==============================================================================
func _on_refresh_pressed():
	if current_browser == null:
		return
	current_browser.reload()
	pass

# ==============================================================================
# Go to previously visited page
# ==============================================================================
func _on_Prev_pressed():
	if current_browser != null:
		current_browser.previous_page()
	pass

# ==============================================================================
# Go to next visited page
# ==============================================================================
func _on_Next_pressed():
	if current_browser != null:
		current_browser.next_page()
	pass

# ==============================================================================
# Select the new desired browser from the list of tabs.
# ==============================================================================
func _on_BrowserList_item_selected(index):
	current_browser = get_browser(str(index))
	if current_browser != null:
		$Panel/VBox/TextureRect.texture = current_browser.get_texture()
		# Update URL bar with current page
		$Panel/VBox/TopBar/URLContainer/TextEdit.text = current_browser.get_url()
	pass

####
#### Bottom menu
####

# ==============================================================================
# Color button pressed: present a pop-up to change the background color
# ==============================================================================
func _on_BGColor_pressed():
	if $ColorPopup.visible:
		$ColorPopup.popup_hide()
	else:
		$ColorPopup.popup_centered(Vector2(0, 0))
	pass

# ==============================================================================
# Color picker changed: inject javascript to change the background color
# ==============================================================================
func _on_ColorPicker_color_changed(color):
	if current_browser != null:
		var js_string = 'document.body.style.background = "#%s"' % color.to_html(false)
		current_browser.execute_javascript(js_string)
	pass

# ==============================================================================
# Radio button pressed: load a page with radio for testing the sound.
# ==============================================================================
func _on_radio_pressed():
	if current_browser != null:
		current_browser.load_url(RADIO_PAGE)
	pass

# ==============================================================================
# Drag and Drop test button pressed: load drag and drop test page.
# ==============================================================================
func _on_dragdrop_pressed():
	if current_browser != null:
		current_browser.load_url("file://" + ProjectSettings.globalize_path(DRAG_DROP_PAGE))
	pass

# ==============================================================================
# Mute/unmute the sound
# ==============================================================================
func _on_mute_pressed(toggled_on):
	if current_browser == null:
		return
	current_browser.set_muted(toggled_on)
	$AudioStreamPlayer2D.stream_paused = toggled_on
	pass

# ==============================================================================
# Block/Unblock ads
# ==============================================================================
func _on_add_blocker_pressed(toggled_on) -> void:
	if current_browser == null:
		return
	current_browser.enable_ad_block(toggled_on)
	pass

####
#### CEF inputs
####

# ==============================================================================
# Get mouse events and broadcast them to CEF
# ==============================================================================
func _on_TextureRect_gui_input(event):
	if current_browser == null:
		return
	if event is InputEventMouseButton:
		# Take focus so keyboard events go to the browser, not the URL bar
		$Panel/VBox/TextureRect.grab_focus()
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			current_browser.set_mouse_wheel_vertical(2, event.shift_pressed,
				event.ctrl_pressed, event.alt_pressed)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			current_browser.set_mouse_wheel_vertical(-2, event.shift_pressed,
				event.ctrl_pressed, event.alt_pressed)
		elif event.button_index == MOUSE_BUTTON_LEFT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				current_browser.set_mouse_left_down()
			else:
				current_browser.set_mouse_left_up()
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				current_browser.set_mouse_right_down()
			else:
				current_browser.set_mouse_right_up()
		else:
			mouse_pressed = event.pressed
			if mouse_pressed:
				current_browser.set_mouse_middle_down()
			else:
				current_browser.set_mouse_middle_up()
	elif event is InputEventMouseMotion:
		# Just move the mouse - don't call set_mouse_left_down() during drag
		# as this interferes with HTML5 drag and drop
		current_browser.set_mouse_moved(event.position.x, event.position.y)
	pass

# ==============================================================================
# Make the CEF browser reacts from keyboard events.
# Only send keyboard events to CEF when the browser texture has focus,
# not when typing in Godot UI elements (URL bar, etc.)
# ==============================================================================
func _input(event):
	if current_browser == null:
		return
	if event is InputEventKey:
		# Check if a Godot UI element has focus (like the URL LineEdit)
		# If so, don't send keyboard events to CEF to avoid conflicts
		var focused = get_viewport().gui_get_focus_owner()
		if focused != null and focused != $Panel/VBox/TextureRect:
			# Let Godot handle keyboard for its own UI elements
			# Only handle global shortcuts like Ctrl+S for save
			if event.is_command_or_control_pressed() && event.pressed && not event.echo:
				if event.keycode == KEY_S:
					if event.shift_pressed:
						# Ctrl+Shift+S: Save as HTML only
						current_browser.save_page(SAVED_PAGE)
					else:
						# Ctrl+S: Save as PDF with all images and resources
						current_browser.save_page_as_pdf(SAVED_PDF)
			return
		# Browser has focus or no UI element has focus - send keys to CEF
		if event.is_command_or_control_pressed() && event.pressed && not event.echo:
			if event.keycode == KEY_S:
				if event.shift_pressed:
					# Ctrl+Shift+S: Save as HTML only
					current_browser.save_page(SAVED_PAGE)
				else:
					# Ctrl+S: Save as PDF with all images and resources
					current_browser.save_page_as_pdf(SAVED_PDF)
		else:
			current_browser.set_key_pressed(
				event.unicode if event.unicode != 0 else event.keycode,
				event.pressed, event.shift_pressed, event.alt_pressed,
				event.is_command_or_control_pressed())
		# Prevent Godot from using arrow keys for UI navigation when browser has focus
		get_viewport().set_input_as_handled()
	pass

# ==============================================================================
# Windows has resized
# ==============================================================================
func _on_texture_rect_resized():
	if current_browser == null:
		return
	current_browser.resize($Panel/VBox/TextureRect.get_size())
	pass

####
#### Godot
####

# ==============================================================================
# Create a single browser named "current_browser" that is attached as child node to $CEF.
# ==============================================================================
func _ready():
	create_default_page()
	create_dragdrop_page()

	# See API.md for more details. CEF Configuration is:
	# {
	#   "incognito": false,
	#   "cache_path": resource_path / "cache",
	#   "root_cache_path": resource_path / "cache",
	#   "browser_subprocess_path": resource_path / SUBPROCESS_NAME,
	#   "log_file": resource_path / "debug.log",
	#   "log_severity": "warning",
	#   "remote_debugging_port": 7777,
	#   "remote_allow_origin": "*",
	#   "exception_stack_size": 5,
	#   "enable_media_stream": false,
	#   "user_gesture_required": true,
	#   "allow_downloads": false,
	#   "download_folder": "res://",
	#   "user_agent": "",
	# }
	#
	# Configure CEF. In incognito mode cache directories not used and in-memory
	# caches are used instead and no data is persisted to disk.
	#
	# artifacts: allows path such as "build" or "res://cef_artifacts/". Note that "res://"
	# will use ProjectSettings.globalize_path but exported projects don't support globalize_path:
	# https://docs.godotengine.org/en/3.5/classes/class_projectsettings.html#class-projectsettings-method-globalize-path
	if !$CEF.initialize({
			"incognito": true,
			"locale": "en-US",
			"enable_media_stream": true,
			"remote_debugging_port": 7777,
			"remote_allow_origin": "*"
		}):
		$Panel/VBox/BottomBar/Info.set_text($CEF.get_error())
		push_error($CEF.get_error())
		return
	print("CEF version: " + $CEF.get_full_version())
	print("You are listening CEF native audio")

	# Logging from the main CEF instance
	$CEF.log_info("This is an example info")
	$CEF.log_warning("This is an example warning")

	# Wait one frame for the texture rect to get its size
	current_browser = await create_browser(HOME_PAGE)
	pass

# ==============================================================================
# $CEF is periodically updated
# ==============================================================================
func _process(_delta):
	pass

# ==============================================================================
# CEF audio will be routed to this Godot stream object.
# ==============================================================================
func _on_routing_audio_pressed(toggled_on):
	if current_browser == null:
		return
	if toggled_on:
		print("You are listening CEF audio routed to Godot and filtered with reverberation effect")
		$AudioStreamPlayer2D.stream = AudioStreamGenerator.new()
		$AudioStreamPlayer2D.stream.set_buffer_length(1)
		$AudioStreamPlayer2D.playing = true
		current_browser.audio_stream = $AudioStreamPlayer2D.get_stream_playback()
	else:
		print("You are listening CEF native audio")
		current_browser.audio_stream = null
		current_browser.set_muted(false)
	$Panel/VBox/BottomBar/Options/Mute.button_pressed = false
	# Not necessary, but, I do not know what, to apply the new mode, the user
	# shall click on the html halt button and click on the html button. To avoid
	# this, we reload the page.
	current_browser.reload()
	pass
