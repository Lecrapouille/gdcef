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
# Bind the JavaScript function to the GDScript method.
# We only need a single instance of the binder since, in this demo, we manage
# a single frame.
# ==============================================================================
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
# Add XP manually from Godot when the Godot button is pressed.
# ==============================================================================
func _on_button_pressed():
	var amount = randi() % 5 + 1
	var js_code = "playerXP += %d; updateDisplay();" % amount
	js_binder.execute_js(js_code)
	pass

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
	if !$CEF.initialize({"incognito": true, "locale": "en-US",
		"remote_debugging_port": 7777, "remote_allow_origin": "*"}):
		push_error("Failed initializing CEF")
		get_tree().quit()
	else:
		push_warning("CEF version: " + $CEF.get_full_version())

	# Create a browser and load the html document with javascript
	var browser = $CEF.create_browser("", $TextureRect, {"javascript": true})
	browser.name = browser_name
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	browser.connect("on_v8_context_created", _on_v8_context_created)
	browser.connect("on_v8_context_destroyed", _on_v8_context_destroyed)
	await get_tree().process_frame # Wait one frame for the texture rect to get its size
	browser.resize($TextureRect.get_size())

	# Configurer la fonction de callback JS avant de charger la page
	js_binder.bind_function("emitJsEvent", self, "_on_js_event")

	# Charger la page HTML
	browser.load_data_uri(_load_html_file(), "text/html")

# ==============================================================================
# Callback when page is loaded - configure JS bindings here
# ==============================================================================
func _on_page_loaded(browser, binder):
	print("The browser " + browser.name + " has loaded document")
	js_binder = binder
	pass

func _on_v8_context_created(browser, context):
	print("V8 context created" + browser.name)
	js_binder.set_context(context)
	js_binder.execute_js("""
		emitJsEvent = function(data) {
			_gdcef_emit_js_event(JSON.stringify(data));
		};
	""")
	pass

func _on_v8_context_destroyed(browser, context):
	print("V8 context destroyed" + browser.name)
	js_binder.set_context(null)
	pass

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
	# Récupérer les valeurs depuis JavaScript
	var xp = js_binder.get_js_variable("playerXP")
	var level = js_binder.get_js_variable("playerLevel")

	# Mettre à jour le TextEdit avec les valeurs formatées
	$TextEdit.text = "Player Level: %s\nPlayer XP: %s" % [level, xp]
	pass
