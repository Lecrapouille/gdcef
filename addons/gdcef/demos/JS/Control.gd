# ==============================================================================
# Basic application made in HTML/JS/CSS with interaction with Godot.
# ==============================================================================
extends Control

# ==============================================================================
# CEF variables
# ==============================================================================
const BROWSER_NAME = "player_stats"
@onready var mouse_pressed: bool = false

# ==============================================================================
# Variables for character stats
# ==============================================================================
@onready var player_name: String = "Anonymous"
@onready var weapon: String = "sword"
@onready var xp: int = 0
@onready var level: int = 1

# ==============================================================================
# Initial character configuration
# ==============================================================================
func _ready():
	initialize_cef()
#	expose_methods()
	pass

# ==============================================================================
# Expose methods to JavaScript via CEF
# ==============================================================================
#func expose_methods():
#	pass

# ==============================================================================
# Change character's weapon
# ==============================================================================
func _change_weapon(new_weapon: String):
	print("Weapon changed to: ", new_weapon)
	weapon = new_weapon
	_update_character_stats()
	pass

# ==============================================================================
# Set character's name
# ==============================================================================
func _set_character_name(new_name: String):
	print("New name: ", new_name)
	player_name = new_name
	_update_character_stats()
	pass

# ==============================================================================
# Modify XP (can be positive or negative)
# ==============================================================================
func _modify_xp(xp_change: int):
	xp += xp_change
	_level_up_check()
	_update_character_stats()
	pass

# ==============================================================================
# Check for level up
# ==============================================================================
func _level_up_check():
	var previous_level = level
	level = 1 + floor(xp / 100) # Simple progression example

	if level > previous_level:
		print("Level up! New level: ", level)
	pass

# ==============================================================================
# Update character statistics
# ==============================================================================
func _update_character_stats():
	var character_info = {
		"name": player_name,
		"weapon": weapon,
		"xp": xp,
		"level": level
	}
	print("Character update: ", character_info)
	pass

# ==============================================================================
# Optional method to get complete character state
# ==============================================================================
func get_character_state() -> Dictionary:
	return {
		"name": player_name,
		"weapon": weapon,
		"xp": xp,
		"level": level
	}

# ==============================================================================
# CEF Callback when a page has ended to load with success.
# ==============================================================================
func _on_page_loaded(browser):
	print("The browser " + browser.name + " has loaded " + browser.get_url())
	$CEF.register_method(self, "change_weapon")
	$CEF.register_method(self, "set_character_name")
	$CEF.register_method(self, "modify_xp")
	pass

# ==============================================================================
# Callback when a page has ended to load with failure.
# Display a load error message using a data: URI.
# ==============================================================================
func _on_page_failed_loading(_err_code, _err_msg, browser):
	$AcceptDialog.title = "Alert!"
	$AcceptDialog.dialog_text = "The browser " + browser.name + " did not load " + browser.get_url()
	$AcceptDialog.popup_centered(Vector2(0, 0))
	$AcceptDialog.show()
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
		if mouse_pressed == true:
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
	if event is InputEventKey:
		get_browser().set_key_pressed(
			event.unicode if event.unicode != 0 else event.keycode,
			event.pressed, event.shift_pressed, event.alt_pressed, event.is_command_or_control_pressed())
	pass

# ==============================================================================
# Split the browser vertically to display two browsers (aka tabs) rendered in
# two separate textures.
# ==============================================================================
func initialize_cef():

	### CEF

	if !$CEF.initialize({"incognito": true, "locale": "en-US",
			"remote_debugging_port": 7777, "remote_allow_origin": "*"}):
		push_error("Failed initializing CEF")
		get_tree().quit()
	else:
		push_warning("CEF version: " + $CEF.get_full_version())
		pass

	### Browser

	var browser = $CEF.create_browser("", $TextureRect, {"javascript": true})
	browser.name = BROWSER_NAME
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	browser.resize($TextureRect.get_size())
	browser.load_data_uri(_load_html_file(), "text/html")
	pass

# ==============================================================================
# Load the HTML file containing the JavaScript code
# ==============================================================================
func _load_html_file():
	var file = FileAccess.open("res://character-management-ui.html", FileAccess.READ)
	var content = file.get_as_text()
	file.close()
	return content

# ==============================================================================
# Get the browser node interacting with the JavaScript code.
# ==============================================================================
func get_browser():
	var browser = $CEF.get_node(BROWSER_NAME)
	if browser == null:
		push_error("Failed getting Godot node '" + name + "'")
		get_tree().quit()
	return browser

# ==============================================================================
# CEF is implicitly updated by this function.
# ==============================================================================
func _process(_delta):
	pass
