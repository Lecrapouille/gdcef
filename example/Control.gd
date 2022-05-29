# Hello CEF: basic application showing how to use CEF inside Godot.
# See our Stigmee application we manage more cases such as mouse and keyboard events
extends Control

# Page iterator
var page = 0

# Change of pages when the timer timeout (around 4s).
func _on_Timer_timeout():
	var pages = [
		"https://github.com/stigmee",
		"https://bitbucket.org/chromiumembedded/cef/wiki/Home",
		"https://docs.godotengine.org/",
		"https://threejs.org/examples/#webgl_animation_keyframes",
	]
	page = (page + 1) % 4
	get_node("CEF/right").load_url(pages[page])

# Callback when a page has ended to load
func _on_page_loaded(node):
	print("The browser " + node.name + " has loaded " + node.get_url())

# Split the browser vertically to display two webpages (into two textures).
func _ready():
	# browser dimension
	var h = get_viewport().size.x
	var w = get_viewport().size.y
	set_position(Vector2(0,0))
	set_size(Vector2(h, w))

	# First tab displaying the first webpage
	var left = $CEF.create_browser("http://chreage.befuse.com/content/WEBGL/ChreageAlpha0-3/index.html", "left", h/2, w)
	$Texture1.set_position(Vector2(0,0))
	$Texture1.set_size(Vector2(h/2, w/2))
	$Texture1.texture = left.get_texture()
	#left.set_viewport(0.25, 0.25, 0.25, 0.25)

	# Second tab displaying the second webpage
	var right = $CEF.create_browser("https://github.com/stigmee", "right", h/2, w)
	$Texture2.set_position(Vector2(h/2,0))
	$Texture2.set_size(Vector2(h/2, w/2))
	$Texture2.texture = right.get_texture()

	# Connect the event when a page has bee loaded and wait X seconds
	# before loading the page
	right.connect("page_loaded", self, "_on_page_loaded")
	$Timer.connect("timeout", self, "_on_Timer_timeout")
	pass

# Note that CEF is implicitely updated by this function.
func _process(delta):
	pass
