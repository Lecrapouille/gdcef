# ==============================================================================
# Basic application made in HTML/JS/CSS with interaction with Godot.
#
# This demo is a simple character management system. The user can change the
# character's name, weapon, and XP. The character's stats are displayed in the
# HTML page. When the user clicks on a button, the JS code is called and the
# Godot script is notified. The Godot script updates the character's stats and
# sends them back to the HTML page.
# ==============================================================================
extends Control

# ==============================================================================
# Binary data types enum
# ==============================================================================
enum BinaryDataType {
	WARRIOR1_SVG, # SVG image
	WARRIOR2_PNG, # PNG image
}

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
@onready var weapon_details: Dictionary = {}
@onready var inventory: Array = []
@onready var current_texture_rect: TextureRect = null

# ==============================================================================
# Initialize CEF and create the GUI as HTML/JS/CSS page.
# ==============================================================================
func _ready():
	initialize_cef()
	pass

# ==============================================================================
# JS callback: Change character's weapon.
# ==============================================================================
func _change_weapon(new_weapon: String):
	print("Weapon changed to: ", new_weapon)
	weapon = new_weapon
	_refresh_page_character_stats()
	pass

# ==============================================================================
# JS callback: Set character's name.
# ==============================================================================
func _set_character_name(new_name: String):
	print("New name: ", new_name)
	player_name = new_name
	_refresh_page_character_stats()
	pass

# ==============================================================================
# Check for level up.
# ==============================================================================
func _level_up_check():
	var previous_level = level
	level = 1 + floor(xp / 100) # Simple progression example

	if level > previous_level:
		print("Level up! New level: ", level)
	pass


# ==============================================================================
# JS callback: Modify XP (can be positive or negative).
# ==============================================================================
func _modify_xp(xp_change: int):
	xp += xp_change
	_level_up_check()
	_refresh_page_character_stats()
	pass

# ==============================================================================
# JS callback: Update weapon details with complex data.
# ==============================================================================
func _update_weapon_details(data: Dictionary):
	print("Received complex weapon data: ", data)
	weapon_details = data

	# Update base weapon
	if data.has("name"):
		weapon = data["name"]

	# Display information on the console
	if data.has("properties") and data["properties"] is Dictionary:
		var props = data["properties"]
		if props.has("damage"):
			print("Weapon damage: ", props["damage"])
		if props.has("magicEffects") and props["magicEffects"] is Array:
			print("Magic effects: ", props["magicEffects"])

	_refresh_page_character_stats()
	pass

# ==============================================================================
# JS callback: Update inventory with array of weapons.
# ==============================================================================
func _update_inventory(weapons_array: Array):
	print("Received weapon inventory: ", weapons_array)
	inventory = weapons_array

	# Display information on the console
	var total_damage = 0
	for weapon_item in weapons_array:
		if weapon_item is Dictionary and weapon_item.has("damage"):
			total_damage += weapon_item["damage"]

	print("Total inventory damage potential: ", total_damage)

	# Update the statistics
	_refresh_page_character_stats()
	pass

# ==============================================================================
# JS callback: Receive binary data type selection from JavaScript
# ==============================================================================
func _select_binary_data_type(type_index: int):
	print("Received binary data type selection: ", type_index)

	# Convert from integer to enum value and send the binary data
	var data_type = type_index
	if type_index < 0 or type_index >= BinaryDataType.size():
		data_type = BinaryDataType.size() - 1
		print("Invalid type index, using default: RED_SQUARE")

	# Send the binary data of the selected type
	_send_binary_data_to_js(data_type)
	pass

# ==============================================================================
# JS callback: Receive binary data from JavaScript
# ==============================================================================
func _receive_binary_data(data: PackedByteArray):
	print("Received binary data from JavaScript")

	# Check the size of the data
	if data.size() <= 0:
		push_error("Invalid binary data: empty buffer")
		return

	print("Successfully received binary data, size: ", data.size())

	# Binary data visualization
	_create_binary_visualization(data)

	# Acknowledgment
	var response_data = {
		"status": "received",
		"size": data.size(),
		"timestamp": Time.get_unix_time_from_system()
	}

	# Send acknowledgment back to JavaScript
	$CEF.get_node(BROWSER_NAME).js_emit("binary_received", response_data)
	print("Acknowledgment sent back to JavaScript")
	pass

# ==============================================================================
# Create a dynamic visualization of binary data
# ==============================================================================
func _create_binary_visualization(binary: PackedByteArray):
	# Create a new Control node for visualization
	var visualization = Control.new()
	visualization.name = "BinaryVisualization"
	visualization.custom_minimum_size = Vector2(300, 500)
	visualization.position = Vector2(20, 20)
	add_child(visualization)

	# Create a background panel with dark theme
	var panel = Panel.new()
	panel.name = "VisualizationPanel"
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)

	# Set dark background
	var panel_style = StyleBoxFlat.new()
	panel_style.bg_color = Color(0.1, 0.1, 0.1)
	panel.add_theme_stylebox_override("panel", panel_style)
	visualization.add_child(panel)

	# Create a VBox for organizing content
	var vbox = VBoxContainer.new()
	vbox.name = "ContentVBox"
	vbox.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	vbox.add_theme_constant_override("separation", 10)
	vbox.add_theme_constant_override("margin_left", 10)
	vbox.add_theme_constant_override("margin_right", 10)
	vbox.add_theme_constant_override("margin_top", 10)
	vbox.add_theme_constant_override("margin_bottom", 10)
	panel.add_child(vbox)

	# Add title
	var title_label = Label.new()
	title_label.text = "Binary Data Visualization"
	title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_label.add_theme_color_override("font_color", Color(1, 1, 1))
	vbox.add_child(title_label)

	# Add size information
	var size_label = Label.new()
	size_label.text = "Binary Size: " + str(binary.size()) + " bytes"
	size_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	size_label.add_theme_color_override("font_color", Color(1, 1, 1))
	vbox.add_child(size_label)

	# Create TextureRect for image display
	var texture_rect = TextureRect.new()
	texture_rect.custom_minimum_size = Vector2(280, 280)
	texture_rect.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	texture_rect.size_flags_vertical = Control.SIZE_EXPAND_FILL
	texture_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	texture_rect.expand_mode = TextureRect.EXPAND_FIT_WIDTH_PROPORTIONAL
	vbox.add_child(texture_rect)
	current_texture_rect = texture_rect

	# Try to create image from binary data
	var img = Image.new()
	var err = img.load_png_from_buffer(binary)
	if err == OK:
		var tex = ImageTexture.create_from_image(img)
		texture_rect.texture = tex
		print("Successfully loaded image: ", img.get_size())
	else:
		print("Failed to load image from binary data: ", err)

	# Add buttons with custom styling
	var button_style = StyleBoxFlat.new()
	button_style.bg_color = Color(0.2, 0.2, 0.2)
	button_style.border_color = Color(0.3, 0.3, 0.3)
	button_style.border_width_left = 1
	button_style.border_width_top = 1
	button_style.border_width_right = 1
	button_style.border_width_bottom = 1

	# Add a close button
	var close_button = Button.new()
	close_button.text = "Close Visualization"
	close_button.custom_minimum_size = Vector2(200, 30)
	close_button.pressed.connect(_on_close_visualization.bind(visualization))
	close_button.add_theme_stylebox_override("normal", button_style)
	close_button.add_theme_color_override("font_color", Color(1, 1, 1))
	vbox.add_child(close_button)

	# Add a save button
	var save_button = Button.new()
	save_button.text = "Save Binary Data"
	save_button.custom_minimum_size = Vector2(200, 30)
	save_button.pressed.connect(_on_save_binary_data.bind(binary))
	save_button.add_theme_stylebox_override("normal", button_style)
	save_button.add_theme_color_override("font_color", Color(1, 1, 1))
	vbox.add_child(save_button)

	# Add a clear button
	var clear_button = Button.new()
	clear_button.text = "Clear All Visualizations"
	clear_button.custom_minimum_size = Vector2(200, 30)
	clear_button.pressed.connect(_on_clear_visualizations)
	clear_button.add_theme_stylebox_override("normal", button_style)
	clear_button.add_theme_color_override("font_color", Color(1, 1, 1))
	vbox.add_child(clear_button)

	# Auto-remove after 30 seconds
	await get_tree().create_timer(30.0).timeout
	if is_instance_valid(visualization) and visualization.is_inside_tree():
		visualization.queue_free()
	pass

# ==============================================================================
# Close button callback for visualization
# ==============================================================================
func _on_close_visualization(visualization: Control):
	if is_instance_valid(visualization):
		# Reset the TextureRect reference if this is the current visualization
		if current_texture_rect and current_texture_rect.is_ancestor_of(visualization):
			current_texture_rect = null
		# Close the visualization
		visualization.queue_free()
	pass

# ==============================================================================
# Save binary data callback
# ==============================================================================
func _on_save_binary_data(binary: PackedByteArray):
	var file = FileAccess.open("user://received_binary.dat", FileAccess.WRITE)
	if file:
		file.store_buffer(binary)
		file.close()
		print("Binary data saved to user://received_binary.dat")
	else:
		push_error("Failed to save binary data")
	pass

# ==============================================================================
# Clear visualization callback
# ==============================================================================
func _on_clear_visualizations():
	# Efface la texture du TextureRect
	if current_texture_rect and is_instance_valid(current_texture_rect):
		current_texture_rect.texture = null
	pass

# ==============================================================================
# Send binary data to JavaScript
# ==============================================================================
func _send_binary_data_to_js(data_type: int):
	print("Sending binary data to JavaScript, type: ", data_type)

	var binary_data: PackedByteArray

	match data_type:
		BinaryDataType.WARRIOR1_SVG:
			# Try loading warrior1.svg
			if FileAccess.file_exists("res://warrior1.svg"):
				var img = Image.load_from_file("res://warrior1.svg")
				if img != null:
					binary_data = img.save_png_to_buffer()
					print("Sending warrior1.svg, binary size: ", binary_data.size())

		BinaryDataType.WARRIOR2_PNG:
			# Try loading warrior2.png
			if FileAccess.file_exists("res://warrior2.png"):
				var img = Image.load_from_file("res://warrior2.png")
				if img != null:
					binary_data = img.save_png_to_buffer()
					print("Sending warrior2.png, binary size: ", binary_data.size())

	# If attempt failed, create a fallback red image
	if binary_data.size() == 0:
		var img = Image.new()
		img.create(16, 16, false, Image.FORMAT_RGBA8)
		img.fill(Color.RED)
		binary_data = img.save_png_to_buffer()
		print("Created fallback red image, binary size: ", binary_data.size())

	# Send data to JavaScript
	if binary_data.size() > 0:
		# Send the binary data with additional information about the type
		var data_info = {
			"type_name": BinaryDataType.keys()[data_type] if data_type < BinaryDataType.size() else "UNKNOWN",
			"type_index": data_type
		}
		$CEF.get_node(BROWSER_NAME).js_emit("binary_data_info", data_info)

		# Then send the actual binary data
		$CEF.get_node(BROWSER_NAME).js_emit("binary_data", binary_data)
		print("Binary data sent to JavaScript")
	else:
		push_error("ERROR: Could not create binary data to send")
	pass

# ==============================================================================
# Refresh the HTML page with the new character statistics.
# ==============================================================================
func _refresh_page_character_stats():
	$CEF.get_node(BROWSER_NAME).js_emit("character_update", _get_character_state())
	pass

# ==============================================================================
# Optional method to get complete character state
# ==============================================================================
func _get_character_state() -> Dictionary:
	var character_info = {
		"name": player_name,
		"weapon": weapon,
		"xp": xp,
		"level": level
	}

	# Add weapon details if available
	if not weapon_details.is_empty():
		character_info["weaponDetails"] = weapon_details

	# Add inventory if it's not empty
	if not inventory.is_empty():
		character_info["inventory"] = inventory

	print("Character update: ", character_info)
	return character_info

# ==============================================================================
# CEF Callback when a page has ended to load with success.
# ==============================================================================
func _on_page_loaded(browser):
	print("The browser " + browser.name + " has loaded " + browser.get_url())

	# Register methods for JS ==> Godot communication
	browser.register_method(self, "_change_weapon")
	browser.register_method(self, "_set_character_name")
	browser.register_method(self, "_modify_xp")
	browser.register_method(self, "_update_weapon_details")
	browser.register_method(self, "_update_inventory")
	browser.register_method(self, "_receive_binary_data")
	browser.register_method(self, "_select_binary_data_type")

	# Send initial character state to the HTML page
	_refresh_page_character_stats()

	# Send information about available binary data types to JavaScript
	var binary_types = []
	for i in range(BinaryDataType.size()):
		binary_types.append({
			"index": i,
			"name": BinaryDataType.keys()[i]
		})
	browser.js_emit("binary_data_types", binary_types)

	# Test binary data transmission
	_send_binary_data_to_js(BinaryDataType.WARRIOR1_SVG)
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
# Split the browser vertically to display two browsers (aka tabs) rendered in
# two separate textures.
# ==============================================================================
func initialize_cef():
	# CEF initialization
	if !$CEF.initialize({
			"incognito": true,
			"remote_debugging_port": 7777,
			"remote_allow_origin": "*"
		}):
		push_error("Failed initializing CEF")
		get_tree().quit()
	else:
		push_warning("CEF version: " + $CEF.get_full_version())
		pass

	# Browser creation: load the local HTML page that will be used as a GUI.
	var browser = $CEF.create_browser("res://character-management-ui.html",
		$TextureRect, {"javascript": true})
	browser.name = BROWSER_NAME
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)
	browser.resize($TextureRect.get_size())
	pass

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
# Get mouse events and broadcast them to CEF
# ==============================================================================
func _on_TextureRect_gui_input(event: InputEvent):
	var current_browser = get_browser()
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			current_browser.set_mouse_wheel_vertical(2)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			current_browser.set_mouse_wheel_vertical(-2)
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
		if mouse_pressed:
			current_browser.set_mouse_left_down()
		current_browser.set_mouse_moved(event.position.x, event.position.y)
	pass

# ==============================================================================
# Make the CEF browser reacts from keyboard events.
# ==============================================================================
func _input(event):
	if event is InputEventKey:
		get_browser().set_key_pressed(
			event.unicode if event.unicode != 0 else event.keycode,
			event.pressed, event.shift_pressed, event.alt_pressed,
			event.is_command_or_control_pressed())
	pass

# ==============================================================================
# CEF is implicitly updated by this function.
# ==============================================================================
func _process(_delta):
	pass
