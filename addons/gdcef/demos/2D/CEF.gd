# ==============================================================================
# Demo based on the initial asset https://godotengine.org/asset-library/asset/127
# Basic application showing how to use CEF inside Godot with a 3D scene and mouse
# and keyboard events.
# ==============================================================================

extends Control

# URL
const DEFAULT_PAGE = "user://default_page.html"
const SAVED_PAGE = "user://saved_page.html"
const HOME_PAGE = "https://github.com/Lecrapouille/gdcef"
const RADIO_PAGE = "http://streaming.radio.co/s9378c22ee/listen"
#const RADIO_PAGE = "https://www.programmes-radio.com/fr/stream-e8BxeoRhsz9jY9mXXRiFTE/ecouter-KPJK"

# The current browser as Godot node
@onready var current_browser = null
# Memorize if the mouse was pressed
@onready var mouse_pressed : bool = false

# ==============================================================================
# Create the home page.
# ==============================================================================
func create_default_page():
	var file = FileAccess.open(DEFAULT_PAGE, FileAccess.WRITE)
	file.store_string("<html><body bgcolor=\"white\"><h2>Welcome to gdCEF !</h2><p>This a generated page.</p></body></html>")
	file.close()
	pass

# ==============================================================================
# Save page as html.
# ==============================================================================
func _on_saving_page(html, brower):
	var path = ProjectSettings.globalize_path(SAVED_PAGE)
	var file = FileAccess.open(SAVED_PAGE, FileAccess.WRITE)
	if (file != null):
		file.store_string(html)
		file.close()
		$AcceptDialog.title = brower.get_url()
		$AcceptDialog.dialog_text = "Page saved at:\n" + path
	else:
		$AcceptDialog.title = "Alert!"
		$AcceptDialog.dialog_text = "Failed creating the file " + path
	$AcceptDialog.popup_centered(Vector2(0,0))
	$AcceptDialog.show()
	pass

# ==============================================================================
# Callback when a page has ended to load with success (200): we print a message
# ==============================================================================
func _on_page_loaded(brower):
	var L = $Panel/VBox/HBox/BrowserList
	var url = brower.get_url()
	L.set_item_text(L.get_selected_id(), url)
	$Panel/VBox/HBox2/Info.set_text(url + " loaded as ID " + brower.name)
	print("Browser named '" + brower.name + "' inserted on list at index " + str(L.get_selected_id()) + ": " + url)
	pass

# ==============================================================================
# Callback when a page has ended to load with failure.
# Display a load error message using a data: URI.
# ==============================================================================
func _on_page_failed_loading(aborted, msg_err, node):
	# FIXME: I dunno why the radio page is considered as canceled by the user
	if node.get_url() == RADIO_PAGE:
		return
	var html = "<html><body bgcolor=\"white\"><h2>Failed to load URL " + node.get_url()
	if aborted:
		html = html + " aborted by the user!</h2></body></html>"
	else:
		html = html + " with error " + msg_err + "!</h2></body></html>"
	node.load_data_uri(html, "text/html")
	pass

# ==============================================================================
# Create a new browser and return it or return null if failed.
# ==============================================================================
func create_browser(url):
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
	var browser = $CEF.create_browser(url, $Panel/VBox/TextureRect, {"javascript":true})
	if browser == null:
		$Panel/VBox/HBox2/Info.set_text($CEF.get_error())
		return null

	# Loading callbacks
	browser.connect("on_html_content_requested", _on_saving_page)
	browser.connect("on_page_loaded", _on_page_loaded)
	browser.connect("on_page_failed_loading", _on_page_failed_loading)

	# Add the URL to the list
	$Panel/VBox/HBox/BrowserList.add_item(url)
	$Panel/VBox/HBox/BrowserList.select($Panel/VBox/HBox/BrowserList.get_item_count() - 1)
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
		$Panel/VBox/HBox2/Info.set_text("Unknown browser with name '" + name + "'")
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
# Go to the URL given by the text edit widget.
# ==============================================================================
func _on_go_pressed():
	if current_browser != null:
		current_browser.load_url($Panel/VBox/HBox/TextEdit.text)
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
		$ColorPopup.popup_centered(Vector2(0,0))
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
# Mute/unmute the sound
# ==============================================================================
func _on_mute_pressed():
	if current_browser == null:
		return
	current_browser.set_muted($Panel/VBox/HBox2/Mute.button_pressed)
	$AudioStreamPlayer2D.stream_paused = $Panel/VBox/HBox2/Mute.button_pressed
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
	if current_browser == null:
		return
	if event is InputEventKey:
		if event.is_command_or_control_pressed() && event.pressed && not event.echo:
			if event.keycode == KEY_S:
				# Will call the callback 'on_html_content_requested'
				current_browser.request_html_content()
			# FIXME copy()/paste() inside a Godot text entry will freeze the application
			# https://github.com/chromiumembedded/cef/issues/3117
			#if event.keycode == KEY_C:
			#	current_browser.copy()
			#elif event.keycode == KEY_V:
			#	current_browser.paste()
			#elif event.keycode == KEY_X:
			#	current_browser.cut()
			#elif event.keycode == KEY_DELETE:
			#	current_browser.delete()
			#elif event.keycode == KEY_Z:
			#	current_browser.undo()
			#elif event.shift_pressed && event.keycode == KEY_Z:
			#	current_browser.redo()
		else:
			current_browser.set_key_pressed(
				event.unicode if event.unicode != 0 else event.keycode,
				event.pressed, event.shift_pressed, event.alt_pressed,
				event.is_command_or_control_pressed())
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
# Create a single briwser named "current_browser" that is attached as child node to $CEF.
# ==============================================================================
func _ready():
	create_default_page()

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
	if !$CEF.initialize({
			"incognito":true,
			"locale":"en-US",
			"enable_media_stream": true
		}):
		$Panel/VBox/HBox2/Info.set_text($CEF.get_error())
		push_error($CEF.get_error())
		return
	print("CEF version: " + $CEF.get_full_version())
	print("You are listening CEF native audio")

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
func _on_routing_audio_pressed():
	if current_browser == null:
		return
	if $Panel/VBox/HBox2/RoutingAudio.button_pressed:
		print("You are listening CEF audio routed to Godot and filtered with reverberation effect")
		$AudioStreamPlayer2D.stream = AudioStreamGenerator.new()
		$AudioStreamPlayer2D.stream.set_buffer_length(1)
		$AudioStreamPlayer2D.playing = true
		current_browser.audio_stream = $AudioStreamPlayer2D.get_stream_playback()
	else:
		print("You are listening CEF native audio")
		current_browser.audio_stream = null
		current_browser.set_muted(false)
	$Panel/VBox/HBox2/Mute.button_pressed = false
	# Not necessary, but, I do not know what, to apply the new mode, the user
	# shall click on the html halt button and click on the html button. To avoid
	# this, we reload the page.
	current_browser.reload()
	pass
