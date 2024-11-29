# ==============================================================================
# Hello CEF: basic application showing how to use CEF inside Godot with a 2D
# scene. This demo manages several browser tabs (one named "left", the second
# "right) on a vertically splitted GUI. A timer makes load new URL. The API can
# manage more cases but in this demo the mouse and the keyboard are not managed.
# ==============================================================================
extends Control

@onready var mouse_pressed : bool = false
@onready var browser = null

# ==============================================================================
# CEF Callback when a page has ended to load with success.
# ==============================================================================
func _on_page_loaded(node):
	print("The browser " + node.name + " has loaded " + node.get_url())

# ==============================================================================
# Callback when a page has ended to load with failure.
# Display a load error message using a data: URI.
# ==============================================================================
func _on_page_failed_loading(aborted, msg_err, node):
	print("The browser " + node.name + " did not load " + node.get_url())
	pass

# ==============================================================================
# Make the CEF browser reacts to mouse events.
# ==============================================================================
func _react_to_mouse_event(event, name):
	var browser = $CEF.get_node(name)
	if browser == null:
		push_error("Failed getting Godot node '" + name + "'")
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

# ==============================================================================
# Get mouse events and broadcast them to CEF
# ==============================================================================
func _on_TextRect_gui_input(event):
	_react_to_mouse_event(event, "browser")
	pass

# ==============================================================================
# Make the CEF browser reacts from keyboard events.
# ==============================================================================
func _input(event):
	if browser == null:
		return

	if event is InputEventKey:
		browser.set_key_pressed(
			event.unicode if event.unicode != 0 else event.keycode, # Godot3: event.scancode,
			event.pressed, event.shift_pressed, event.alt_pressed, event.is_command_or_control_pressed())
	pass

# ==============================================================================
# Split the browser vertically to display two browsers (aka tabs) rendered in
# two separate textures. Note viewport on texture is not tottally functional.
# ==============================================================================
func _ready():
	var h = get_viewport().size.x
	var w = get_viewport().size.y
	set_position(Vector2(0,0))
	set_size(Vector2(h, w))

	### CEF ####################################################################

	if !$CEF.initialize({"incognito":true, "locale":"en-US"}):
		push_error("Failed initializing CEF")
		get_tree().quit()
	else:
		push_warning("CEF version: " + $CEF.get_full_version())
		pass

	### Browser ###############################################################

	# Wait one frame for the texture rect to get its size
	await get_tree().process_frame
	var browser = $CEF.create_browser("res://binding_js.html", $TextRectLeft, {"javascript": true})
	browser.name = "browser"

	# Connect the event when a page has bee loaded and wait 6 seconds before
	# loading the page.
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	pass

# ==============================================================================
# CEF is implicitely updated by this function.
# ==============================================================================
func _process(_delta):
	pass
