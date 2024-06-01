# ==============================================================================
# Demo based on the initial asset https://godotengine.org/asset-library/asset/127
# Basic application showing how to use CEF inside Godot with a 3D scene and mouse
# and keyboard events.
# ==============================================================================

extends Control

# Name of the browser
const browser_name = "browser"

# Page with sound
# "https://www.programmes-radio.com/fr/stream-e8BxeoRhsz9jY9mXXRiFTE/ecouter-KPJK"
const RADIO_URL = "http://streaming.radio.co/s9378c22ee/listen"
const HOME_URL = "https://github.com/Lecrapouille/gdcef"

# Memorize if the mouse was pressed
@onready var mouse_pressed : bool = false

# ==============================================================================
# Home button pressed: get the browser node and load a new page.
# ==============================================================================
func _on_Home_pressed():
	var browser = $CEF.get_node(browser_name)
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node " + browser_name)
		return
	browser.load_url(HOME_URL)
	pass

# ==============================================================================
# Go to previously visited page
# ==============================================================================
func _on_Prev_pressed():
	var browser = $CEF.get_node(browser_name)
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node " + browser_name)
		return
	browser.previous_page()
	pass

# ==============================================================================
# Go to next page
# ==============================================================================
func _on_Next_pressed():
	var browser = $CEF.get_node(browser_name)
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node " + browser_name)
		return
	browser.next_page()
	pass

# ==============================================================================
# Callback when a page has ended to load: we print a message
# ==============================================================================
func _on_page_loaded(node):
	$Panel/Label.set_text(node.name + ": page " + node.get_url() + " loaded")

# ==============================================================================
# Callback when a page has ended to load with failure.
# Display a load error message using a data: URI.
# ==============================================================================
func _on_page_failed_loading(aborted, msg_err, node):
	print("The browser " + node.name + " did not load " + node.get_url())
	pass

# ==============================================================================
# On new URL entered
# ==============================================================================
func _on_TextEdit_text_changed(new_text):
	var browser = $CEF.get_node(browser_name)
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node " + browser_name)
		return
	browser.load_url(new_text)

# ==============================================================================
# Get mouse events and broadcast them to CEF
# ==============================================================================
func _on_TextureRect_gui_input(event):
	var browser = $CEF.get_node(browser_name)
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node " + browser_name)
		return
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			browser.set_mouse_wheel_vertical(2)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			browser.set_mouse_wheel_vertical(-2)
		elif event.button_index == MOUSE_BUTTON_LEFT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				browser.set_mouse_left_down()
			else:
				browser.set_mouse_left_up()
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				browser.set_mouse_right_down()
			else:
				browser.set_mouse_right_up()
		else:
			mouse_pressed = event.pressed
			if mouse_pressed:
				browser.set_mouse_middle_down()
			else:
				browser.set_mouse_middle_up()
	elif event is InputEventMouseMotion:
		if mouse_pressed == true :
			browser.set_mouse_left_down()
		browser.set_mouse_moved(event.position.x, event.position.y)
	pass

# ==============================================================================
# Make the CEF browser reacts from keyboard events.
# ==============================================================================
func _input(event):
	var browser = $CEF.get_node(browser_name)
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node " + browser_name)
		return
	if event is InputEventKey:
		browser.set_key_pressed(
			event.unicode if event.unicode != 0 else event.keycode, # Godot3: event.scancode,
			event.pressed, event.shift_pressed, event.alt_pressed, event.is_command_or_control_pressed())
	pass

# ==============================================================================
# Create a single briwser named "browser_name" that is attached as child node to $CEF.
# ==============================================================================
func _ready():
	# See API.md for more details. CEF Configuration is:
	#   resource_path := {"artifacts", CEF_ARTIFACTS_FOLDER}
	#   resource_path := {"exported_artifacts", application_real_path()}
	#   {"incognito":false}
	#   {"cache_path", resource_path / "cache"}
	#   {"root_cache_path", resource_path / "cache"}
	#   {"browser_subprocess_path", resource_path / SUBPROCESS_NAME }
	#   {"log_file", resource_path / "debug.log"}
	#   {log_severity", "warning"}
	#   {"remote_debugging_port", 7777}
	#   {"exception_stack_size", 5}
	#   {"enable_media_stream", false}
	#
	# Configurate CEF. In incognito mode cache directories not used and in-memory
	# caches are used instead and no data is persisted to disk.
	#
	# artifacts: allows path such as "build" or "res://cef_artifacts/". Note that "res://"
	# will use ProjectSettings.globalize_path but exported projects don't support globalize_path:
	# https://docs.godotengine.org/en/3.5/classes/class_projectsettings.html#class-projectsettings-method-globalize-path
	if !$CEF.initialize({"incognito":true, "locale":"en-US"}):
		push_error($CEF.get_error())
		get_tree().quit()
		return
	print("CEF version: " + $CEF.get_full_version())

	# Wait one frame for the texture rect to get its size
	await get_tree().process_frame

	# See API.md for more details. Browser configuration is:
	#   {"frame_rate", 30}
	#   {"javascript", true}
	#   {"javascript_close_windows", false}
	#   {"javascript_access_clipboard", false}
	#   {"javascript_dom_paste", false}
	#   {"image_loading", true}
	#   {"databases", true}
	#   {"webgl", true}
	var browser = $CEF.create_browser(RADIO_URL, $Panel/TextureRect, {"javascript":true})
	browser.name = browser_name
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	browser.set_zoom_level(0.05)

	# 3D sound
	get_tree().get_root().print_tree_pretty()
	var player = get_node("/root/GUIin3D/Background/Cube2/AudioStreamPlayer3D")
	player.stream = AudioStreamGenerator.new()
	player.stream.set_buffer_length(1)
	player.playing = true
	browser.audio_stream = player.get_stream_playback()
	pass

# ==============================================================================
# $CEF is periodically updated
# ==============================================================================
func _process(_delta):
	pass
