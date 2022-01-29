extends Control

func _input(event):
	if event is InputEventMouseButton:
		get_node("cef1").on_mouse_left_click()
	elif event is InputEventMouseMotion:
		get_node("cef1").on_mouse_moved(event.position.x, event.position.y)

func _ready():
	var cef = get_node("cef1")
	cef.load_url("https://youtu.be/cVCfu_KPiR8")
	cef.reshape(800, 600)
	var tex1 = get_node("texture1")
	tex1.set_size(Vector2(800, 600))
	tex1.texture = cef.get_texture()
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	get_node("cef1").do_message_loop_work()
	pass
