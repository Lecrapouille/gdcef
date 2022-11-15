# ==============================================================================
# Hello CEF: basic application showing how to use CEF inside Godot with a 2D
# scene. This demo manages several browser tabs (one named "left", the second
# "right) on a vertically splitted GUI. A timer makes load new URL. The API can
# manage more cases but in this demo the mouse and the keyboard are not managed.
# ==============================================================================
extends Control

# ==============================================================================
# Hold URLs we want to load.
var pages = [
	"https://github.com/Lecrapouille/gdcef",
	"https://bitbucket.org/chromiumembedded/cef/wiki/Home",
	"https://docs.godotengine.org/",
	"https://threejs.org/examples/#webgl_animation_keyframes",
	"http://chreage.befuse.com/content/WEBGL/ChreageAlpha0-3/index.html"
]
# Iterator on the array holding URLs.
var iterator = 0

# ==============================================================================
# Timer callback: every 6 seconds load a new webpage.
# ==============================================================================
func _on_Timer_timeout():
	iterator = (iterator + 1) % 4
	get_node("CEF/right").load_url(pages[iterator])

# ==============================================================================
# CEF Callback when a page has ended to load with success.
# ==============================================================================
func _on_page_loaded(node):
	print("The browser " + node.name + " has loaded " + node.get_url())

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

	# First browser tab is displaying the first webpage.
	var resource_path = ProjectSettings.globalize_path("res://build/")
	print(resource_path)
	var success = $CEF.initialize(resource_path)
	print("SUCCESS: ", success)
	var left = $CEF.create_browser(pages[3], "left", h/2, w, {"webgl": false})
	$Texture1.set_position(Vector2(0,0))
	$Texture1.set_size(Vector2(h/2, w/2))
	$Texture1.texture = left.get_texture()
	#left.set_viewport(0.25, 0.25, 0.25, 0.25)

	# Second browser tab is displaying the second webpage and the timer will
	# make it load a new URL.
	var right = $CEF.create_browser(pages[0], "right", h/2, w, {"javascript": false})
	$Texture2.set_position(Vector2(h/2,0))
	$Texture2.set_size(Vector2(h/2, w/2))
	$Texture2.texture = right.get_texture()

	# Connect the event when a page has bee loaded and wait 6 seconds before
	# loading the page.
	right.connect("page_loaded", self, "_on_page_loaded")
	$Timer.connect("timeout", self, "_on_Timer_timeout")
	pass

# ==============================================================================
# CEF is implicitely updated by this function.
# ==============================================================================
func _process(_delta):
	pass
