extends Control

# ==============================================================================
# CEF variables
# ==============================================================================
const BROWSER_NAME = "test_bindings"
@onready var browser = null
@onready var mouse_pressed: bool = false

# ==============================================================================
# UI variables
# ==============================================================================
@onready var results_label: RichTextLabel = %ResultsLabel

# ==============================================================================
# Test bindings: receives data from JavaScript and sends it back.
# This callback shall be registered with the browser:
# browser.register_method(Callable(self, "test_bindings"))
# ==============================================================================
func test_bindings(data: Variant):
	results_label.text = "Godot Test Results:\n"
	results_label.text += "Test - Received: " + str(data) + "\n"
	# If the value is a string 'timeout', do not respond to simulate a timeout
	if typeof(data) == TYPE_STRING and data == "timeout":
		results_label.text += "Test - Simulating timeout...\n"
		return
	# Send the value to JavaScript
	results_label.text += "Test - Sending: " + str(data) + "\n"
	%CEF.get_node(BROWSER_NAME).js_emit("test_result", data)

# ==============================================================================
# Initialize CEF and create the GUI as HTML/JS/CSS page.
# ==============================================================================
func _ready():
	initialize_cef()
	results_label.text = "Godot Test Results:"

# ==============================================================================
# Initialize CEF and create the browser
# ==============================================================================
func initialize_cef():
	# CEF initialization
	if !%CEF.initialize({
			"incognito": true,
			"remote_debugging_port": 7777,
			"remote_allow_origin": "*"
		}):
		push_error("Failed initializing CEF")
		get_tree().quit()
	else:
		push_warning("CEF version: " + %CEF.get_full_version())

	# Browser creation: load the local HTML page that will be used for tests
	browser = %CEF.create_browser("res://test_bindings.html",
		%TextureRect, {"javascript": true})
	browser.name = BROWSER_NAME
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	browser.resize(%TextureRect.get_size())

# ==============================================================================
# CEF Callback when a page has ended to load with success.
# ==============================================================================
func _on_page_loaded(browser):
	print("The browser " + browser.name + " has loaded " + browser.get_url())

	# Register our test method with the browser
	browser.register_method(self, "test_bindings")

# ==============================================================================
# Callback when a page has ended to load with failure.
# ==============================================================================
func _on_page_failed_loading(_err_code, _err_msg, browser):
	var error = "The browser " + browser.name + " did not load " + browser.get_url()
	results_label.text = error
	push_error(error)

# ==============================================================================
# Resize the browser when the texture rect is resized
# ==============================================================================
func _on_texture_rect_resized():
	if browser == null:
		return
	browser.resize(%TextureRect.get_size())

# ==============================================================================
# Handle mouse events
# ==============================================================================
func _on_texture_rect_gui_input(event: InputEvent) -> void:
	if browser == null:
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
		if mouse_pressed:
			browser.set_mouse_left_down()
		browser.set_mouse_moved(event.position.x, event.position.y)
	pass

# ==============================================================================
# CEF is implicitly updated by this function.
# ==============================================================================
func _process(_delta):
	pass
