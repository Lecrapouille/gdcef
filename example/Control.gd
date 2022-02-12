extends Control

# Change of pages when the timer timeout
var page = 0
func _on_Timer_timeout():
	var pages = [ "https://cryptozombies.io/fr/",
				  "https://phreda4.github.io/",
				  "https://docs.godotengine.org/fi/stable/tutorials/plugins/gdnative/gdnative-cpp-example.html"
				]
	get_node("CEF/right").load_url(pages[page])
	page = (page + 1) % 3

func _on_page_loaded(node):
	print("The browser " + node.name + " has loaded " + node.get_url())

func _ready():
	# Reshape objects
	var h = get_viewport().size.x
	var w = get_viewport().size.y
	set_position(Vector2(0,0))
	set_size(Vector2(h, w))
	$Texture1.set_position(Vector2(0,0))
	$Texture1.set_size(Vector2(h/2, w/2))
	$Texture2.set_position(Vector2(h/2,0))
	$Texture2.set_size(Vector2(h/2, w/2))

	# First tab
	var left = $CEF.create_browser("left", "https://ibob.bg/blog/")
	$Texture1.texture = left.get_texture()
	left.set_size(h/2, w)
	#left.set_viewport(0.25, 0.25, 0.25, 0.25)

	# Second tab
	var right = $CEF.create_browser("right", "https://phreda4.github.io/")
	right.set_size(h/2, w)
	$Texture2.texture = right.get_texture()

	# Connect the event when a page has bee loaded and wait X seconds
	# before loading the page 
	right.connect("page_loaded", self, "_on_page_loaded")
	$Timer.connect("timeout", self, "_on_Timer_timeout")
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
