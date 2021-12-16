extends Control

func _input(event):
	if event is InputEventMouseButton:
		get_node("cef1").on_mouse_click(event.button_index, event.pressed)
	elif event is InputEventMouseMotion:
		get_node("cef1").on_mouse_moved(event.position.x, event.position.y)

func _ready():
	var cef = get_node("cef1")
	cef.reshape(800, 600)
	cef.url("https://youtu.be/cVCfu_KPiR8")
	get_node("texture1").texture = cef.get_texture()
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	get_node("cef1").DoMessageLoop()
	pass
