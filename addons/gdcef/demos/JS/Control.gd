# ==============================================================================
# Basic application showing how to interact with JavaScript from GDScript. The
# JavaScript code is embedded in a local HTML file. The HTML display 2 buttons
# to interact with the GDScript code. One button is used to gain XP and the other
# one to level up. When the player level up, a 30% chance of bonus XP is given.
# Godot is notified of the event by the JavaScript code. This demo also shows the
# interaction from Godot to JavaScript.
# ==============================================================================
extends Control

@onready var mouse_pressed: bool = false
const browser_name = "player_stats"


# ==============================================================================
# Bind the JavaScript function to the GDScript method
# ==============================================================================
@onready var bound_variable = GodotJSBinder.new().bind_variable("playerXP", 0)
@onready var js_binder = GodotJSBinder.new()

# ==============================================================================
# Callback from the JS code when one of the HTML buttons has been clicked.
# The JS code will notify the GDScript code with the type of the event (xp_gained
# or level_up) and the value associated to the event.
# ==============================================================================
func _on_js_event(event_data):
	match event_data.type:
		"xp_gained":
			_handle_xp_gain(event_data.value)
		"level_up":
			_handle_level_up(event_data.level)

func _handle_xp_gain(xp_amount):
	print("XP gained from JS: ", xp_amount)
	if randf() < 0.3: # 30% chance of bonus
		var bonus = randi() % 5 + 1
		print("Bonus XP: ", bonus)
		var js_code = "updateFromGDScript({type: 'bonus_xp', value: %d})" % bonus
		js_binder.execute_js(js_code)

func _handle_level_up(new_level):
	print("Level up from JS: ", new_level)
	var reward = new_level * 100
	print("Level up reward: ", reward)

# ==============================================================================
# Optional function to add XP manually from Godot when the Godot button is pressed
# ==============================================================================
func add_xp_from_godot(amount):
	var js_code = "playerXP += %d; updateDisplay();" % amount
	js_binder.execute_js(js_code)

func _on_button_pressed():
	add_xp_from_godot(10)
	pass

# ==============================================================================
# CEF Callback when a page has ended to load with success.
# ==============================================================================
func _on_page_loaded(node):
	print("The browser " + node.name + " has loaded document")

# ==============================================================================
# Callback when a page has ended to load with failure.
# Display a load error message using a data: URI.
# ==============================================================================
func _on_page_failed_loading(aborted, msg_err, node):
	$AcceptDialog.title = "Alert!"
	$AcceptDialog.dialog_text = "The browser " + node.name + " did not load " + node.get_url()
	$AcceptDialog.popup_centered(Vector2(0, 0))
	$AcceptDialog.show()
	pass

# ==============================================================================
# Get the browser node interacting with the JavaScript code.
# ==============================================================================
func get_browser():
	var browser = $CEF.get_node(browser_name)
	if browser == null:
		push_error("Failed getting Godot node '" + name + "'")
		get_tree().quit()
	return browser

# ==============================================================================
# Make the CEF browser reacts to mouse events.
# ==============================================================================
func _react_to_mouse_event(event, name):
	var browser = get_browser()
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
	_react_to_mouse_event(event, browser_name)
	pass

# ==============================================================================
# Make the CEF browser reacts from keyboard events.
# ==============================================================================
func _input(event):
	if event is InputEventKey:
		get_browser().set_key_pressed(
			event.unicode if event.unicode != 0 else event.keycode, # Godot3: event.scancode,
			event.pressed, event.shift_pressed, event.alt_pressed, event.is_command_or_control_pressed())
	pass

# ==============================================================================
# Split the browser vertically to display two browsers (aka tabs) rendered in
# two separate textures. Note viewport on texture is not tottally functional.
# ==============================================================================
func _ready():
	$TextureRect.set_size(Vector2(400, 400))

	# Init CEF
	if !$CEF.initialize({"incognito": true, "locale": "en-US"}):
		push_error("Failed initializing CEF")
		get_tree().quit()
	else:
		push_warning("CEF version: " + $CEF.get_full_version())

	# Create a browser and load the html document with javascript
	var browser = $CEF.create_browser("", $TextureRect, {"javascript": true})
	browser.name = browser_name
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	await get_tree().process_frame # Wait one frame for the texture rect to get its size
	browser.resize($TextureRect.get_size())
	browser.load_data_uri(_load_html_file(), "text/html")
	
	#
	js_binder.bind_function("emitJsEvent", self, "_on_js_event")

# ==============================================================================
# Load the HTML file containing the JavaScript code
# ==============================================================================
func _load_html_file():
	var file = FileAccess.open("res://binding_js.html", FileAccess.READ)
	var content = file.get_as_text()
	file.close()
	return content

# ==============================================================================
# CEF is implicitly updated by this function.
# ==============================================================================
func _process(_delta):
#	$TextEdit.text = "Player XP: " + bound_variable.get_js_variable("playerXP")
	pass
