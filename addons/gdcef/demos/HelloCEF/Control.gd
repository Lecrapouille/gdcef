# ==============================================================================
# Hello CEF: basic application showing how to use CEF inside Godot with a 2D
# scene. This demo manages several browser tabs (one named "left", the second
# "right) on a vertically splitted GUI. A timer makes load new URL. The API can
# manage more cases but in this demo the mouse and the keyboard are not managed.
# ==============================================================================
extends Control

# ==============================================================================
# Hold URLs we want to load.
const pages = [
	"https://github.com/Lecrapouille/gdcef",
	"https://bitbucket.org/chromiumembedded/cef/wiki/Home",
	"https://docs.godotengine.org/",
	"https://threejs.org/examples/#webgl_animation_keyframes",
	"http://chreage.befuse.com/content/WEBGL/ChreageAlpha0-3/index.html",
	"https://www.localeplanet.com/support/browser.html"
]
# Iterator on the array holding URLs.
@onready var iterator = 0

# The left browser is allowed for mouse and keyboard interaction.
# The right browser is disable because we are automatically switching of pages.
@onready var active_browser = "left"

# Memorize if the mouse was pressed
@onready var mouse_pressed : bool = false

# ==============================================================================
# Timer callback: every 6 seconds load a new webpage.
# ==============================================================================
func _on_Timer_timeout():
	iterator = (iterator + 1) % pages.size()
	get_node("CEF/right").load_url(pages[iterator])

# ==============================================================================
# CEF Callback when a page has ended to load with success.
# ==============================================================================
func _on_page_loaded(node):
	print("The browser " + node.name + " has loaded " + node.get_url())

# ==============================================================================
# Get mouse events and broadcast them to CEF
# ==============================================================================
func _on_Texture1_gui_input(event):
	var browser = $CEF.get_node("left")
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node 'left'")
		return
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			browser.on_mouse_wheel_vertical(2)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			browser.on_mouse_wheel_vertical(-2)
		elif event.button_index == MOUSE_BUTTON_LEFT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				browser.on_mouse_left_down()
			else:
				browser.on_mouse_left_up()
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				browser.on_mouse_right_down()
			else:
				browser.on_mouse_right_up()
		else:
			mouse_pressed = event.pressed
			if mouse_pressed:
				browser.on_mouse_middle_down()
			else:
				browser.on_mouse_middle_up()
	elif event is InputEventMouseMotion:
		if mouse_pressed == true :
			browser.on_mouse_left_down()
		browser.on_mouse_moved(event.position.x, event.position.y)
	pass

# ==============================================================================
# Make the CEF browser reacts from keyboard events.
# ==============================================================================
func _input(event):
	var browser = $CEF.get_node("left")
	if browser == null:
		$Panel/Label.set_text("Failed getting Godot node 'left'")
		return
	if event is InputEventKey:
		browser.on_key_pressed(
			event.unicode if event.unicode != 0 else event.keycode, # Godot3: event.scancode,
			event.pressed, event.shift_pressed, event.alt_pressed, event.is_command_or_control_pressed())
	pass

# ==============================================================================
# Split the browser vertically to display two browsers (aka tabs) rendered in
# two separate textures. Note viewport on texture is not tottally functional.
# ==============================================================================
func _ready():
	# Set application dimension
	var h = get_viewport().size.x
	var w = get_viewport().size.y
	set_position(Vector2(0,0))
	set_size(Vector2(h, w))

	### CEF ####################################################################

	# Configuration are:
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
	#
	# Configurate CEF. In incognito mode cache directories not used and in-memory
	# caches are used instead and no data is persisted to disk.
	#
	# artifacts: allows path such as "build" or "res://cef_artifacts/". Note that "res://"
	# will use ProjectSettings.globalize_path but exported projects don't support globalize_path:
	# https://docs.godotengine.org/en/3.5/classes/class_projectsettings.html#class-projectsettings-method-globalize-path
	var resource_path = "res://cef_artifacts/"
	if !$CEF.initialize({"artifacts":resource_path, "incognito":true, "locale":"en-US"}):
		push_error("Failed initializing CEF")
		get_tree().quit()
	else:
		push_warning("CEF version: " + $CEF.get_full_version())
		pass

	### Browsers ###############################################################

	# wait one frame for the texture rect to get its size
	await get_tree().process_frame
	# Left browser is displaying the first webpage with a 3D scene, we are
	# enabling webgl. Other default configuration are:
	#   {"frame_rate", 30}
	#   {"javascript", true}
	#   {"javascript_close_windows", false}
	#   {"javascript_access_clipboard", false}
	#   {"javascript_dom_paste", false}
	#   {"image_loading", true}
	#   {"databases", true}
	#   {"webgl", true}
	var left = $CEF.create_browser(pages[4], "left", h/2, w, {})
	$Texture1.set_position(Vector2(0,0))
	$Texture1.set_size(Vector2(h/2, w/2))
	$Texture1.texture = left.get_texture()
	#left.set_viewport(0.25, 0.25, 0.25, 0.25)

	# Right browser is displaying the second webpage and the timer will
	# make it load a new URL.
	var right = $CEF.create_browser(pages[0], "right", h/2, w, {})
	$Texture2.set_position(Vector2(h/2,0))
	$Texture2.set_size(Vector2(h/2, w/2))
	$Texture2.texture = right.get_texture()

	# Connect the event when a page has bee loaded and wait 6 seconds before
	# loading the page.
	right.connect("page_loaded", Callable(self, "_on_page_loaded"))
	var _err = $Timer.connect("timeout", Callable(self, "_on_Timer_timeout"))
	pass

# ==============================================================================
# CEF is implicitely updated by this function.
# ==============================================================================
func _process(_delta):
	pass
