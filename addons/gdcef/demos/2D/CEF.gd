# ==============================================================================
# Demo based on the initial asset https://godotengine.org/asset-library/asset/127
# Basic application showing how to use CEF inside Godot with a 3D scene and mouse
# and keyboard events.
# ==============================================================================

extends Control

# Default pages
const DEFAULT_PAGE = "https://mytuner-radio.com/fr/radio/kpjk-radio-472355/"
const HOME_PAGE = "https://github.com/Lecrapouille/gdcef"

# The current browser as Godot node
@onready var current_browser = null

# Memorize if the mouse was pressed
@onready var mouse_pressed : bool = false

# ==============================================================================
# Create a new browser and return it or return null if failed.
# ==============================================================================
func create_browser(url):
	var browserName = str($Panel/VBox/HBox/BrowserList.get_item_count())
	print("Create browser " + browserName + ": " + url)
	# wait one frame for the texture rect to get its size
	await get_tree().process_frame
	var browserSize = $Panel/VBox/TextureRect.get_size()
	var browser = $CEF.create_browser(url, browserName, browserSize.x, browserSize.y, {"javascript":true})
	if browser == null:
		$Panel/VBox/HBox/Info.set_text($CEF.get_error())
		return null
	$Panel/VBox/HBox/BrowserList.add_item(url)
	$Panel/VBox/HBox/BrowserList.select($Panel/VBox/HBox/BrowserList.get_item_count() - 1)
	browser.connect("page_loaded", _on_page_loaded)
	$Panel/VBox/TextureRect.texture = browser.get_texture()
	return browser

# ==============================================================================
# Search the desired by its name. Return the browser as Godot node or null if
# not found.
# ==============================================================================
func get_browser(browserName):
	if not $CEF.is_alive():
		return null
	var browser = $CEF.get_node(browserName)
	if browser == null:
		$Panel/VBox/HBox/Info.set_text("Unknown browser " + browserName)
		return null
	return browser

# ==============================================================================
# Select the new desired browser from the list of tabs.
# ==============================================================================
func _on_BrowserList_item_selected(index):
	current_browser = get_browser(str(index))
	if current_browser != null:
		$Panel/VBox/TextureRect.texture = current_browser.get_texture()
	pass

# ==============================================================================
# 'M' button pressed: mute/unmute the sound
# ==============================================================================
func _on_Mute_pressed():
	if current_browser != null:
		current_browser.set_muted(not current_browser.is_muted())
	pass # Replace with function body.

# ==============================================================================
# '+' button pressed: create a new browser node.
# ==============================================================================
func _on_Add_pressed():
	var browser = await create_browser(DEFAULT_PAGE)
	if browser != null:
		current_browser = browser
	pass

# ==============================================================================
# Home button pressed: get the browser node and load a new page.
# ==============================================================================
func _on_Home_pressed():
	if current_browser != null:
		current_browser.load_url(HOME_PAGE)
	pass

# ==============================================================================
# Color button pressed: present a pop-up to change the background color
# ==============================================================================
func _on_BGColor_pressed():
	if $ColorPopup.visible:
		$ColorPopup.popup_hide()
	else:
		$ColorPopup.popup_centered(Vector2(0,0))

# ==============================================================================
# Color picker changed: inject javascript to change the background color
# ==============================================================================
func _on_ColorPicker_color_changed(color):
	if current_browser != null:
		var js_string = 'document.body.style.background = "#%s"' % color.to_html(false)
		current_browser.execute_javascript(js_string)

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
# Callback when a page has ended to load: we print a message
# ==============================================================================
func _on_page_loaded(node):
	var L = $Panel/VBox/HBox/BrowserList
	var url = node.get_url()
	L.set_item_text(L.get_selected_id(), url)
	$Panel/VBox/HBox/Info.set_text("Tab " + node.name + ": " + url + " loaded")
	print("Browser " + str(L.get_selected_id()) + ": " + url)

# ==============================================================================
# On new URL entered
# ==============================================================================
func _on_TextEdit_text_changed(new_text):
	if current_browser != null:
		current_browser.load_url(new_text)

# ==============================================================================
# Get mouse events and broadcast them to CEF
# ==============================================================================
func _on_TextureRect_gui_input(event):
	if current_browser == null:
		return
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			current_browser.on_mouse_wheel_vertical(2)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			current_browser.on_mouse_wheel_vertical(-2)
		elif event.button_index == MOUSE_BUTTON_LEFT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				current_browser.on_mouse_left_down()
			else:
				current_browser.on_mouse_left_up()
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			mouse_pressed = event.pressed
			if mouse_pressed:
				current_browser.on_mouse_right_down()
			else:
				current_browser.on_mouse_right_up()
		else:
			mouse_pressed = event.pressed
			if mouse_pressed:
				current_browser.on_mouse_middle_down()
			else:
				current_browser.on_mouse_middle_up()
	elif event is InputEventMouseMotion:
		if mouse_pressed:
			current_browser.on_mouse_left_down()
		current_browser.on_mouse_moved(event.position.x, event.position.y)
	pass

# ==============================================================================
# Make the CEF browser reacts from keyboard events.
# ==============================================================================
func _input(event):
	if current_browser == null:
		return
	if event is InputEventKey:
		current_browser.on_key_pressed(
			event.unicode if event.unicode != 0 else event.keycode, # Godot3: event.scancode,
			event.pressed, event.shift_pressed, event.alt_pressed, event.is_command_or_control_pressed())
	pass

# ==============================================================================
# Windows has resized
# ==============================================================================
func _on_texture_rect_resized():
	if current_browser == null:
		return
	current_browser.resize($Panel/VBox/TextureRect.get_size().x, $Panel/VBox/TextureRect.get_size().y)
	pass

# ==============================================================================
# Create a single briwser named "current_browser" that is attached as child node to $CEF.
# ==============================================================================
func _ready():
	if !$CEF.initialize({"locale":"en-US"}):
		$Panel/VBox/HBox/Info.set_text($CEF.get_error())
		push_error($CEF.get_error())
		return
	push_warning("CEF version: " + $CEF.get_full_version())
	current_browser = await create_browser(HOME_PAGE)
	pass

# ==============================================================================
# $CEF is periodically updated
# ==============================================================================
func _process(_delta):
	pass
