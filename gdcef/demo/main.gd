extends Spatial


var time_elapsed = 0
var keys_typed = []
var quit = false
# Will initialize CEF
var cef = GDCef.new()
var thread_start
var thread_loop
var thread_paint
var thread_browser
var thread_loopwork

var b


# Called when the node enters the scene tree for the first time.
func _ready():
	
	thread_start = Thread.new()
	thread_loop = Thread.new()
	thread_paint = Thread.new()
	thread_browser = Thread.new()
	thread_loopwork = Thread.new()
	
	# show information about window handle
	print(OS.get_name())
	print(OS.get_native_handle(2))
	
	#thread_start.start(self, "_my_cef_start")
	_my_cef_start()
	#_initialize()
	thread_loop.start(self, "_my_loop_thread")

	

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	
	if b:
		# Call DoMessageLoopWork
		_my_work()
		# Then update the texture
		$BrowserTexture.texture = b.get_texture()
		$BrowserTexture.update()
	
func _my_browser():
	
	print("[_my_browser] new")
	b = BrowserView.new()

	#print("[_my_browser] load_url")
	b.load_url("https://www.youtube.com/watch?v=hW7m-bemWYw")
	#print("[_my_browser] reshape")
	b.reshape(1000,600)


func _my_loop():
	
	print("[_my_loop] Calling my loop")
	while(true):
		$BrowserTexture.texture = b.get_texture()
		$BrowserTexture.update()

#	print("_my_loop END")	

func _my_work():

	cef.do_message_loop_work()

func _my_loop_thread():
	
	cef.run_message_loop()

func _my_work_thread():
	
	while (true):
		cef.do_message_loop_work()


func _on_Start_pressed():
	
	if not b:
		_my_browser()
		#thread_loopwork.start(self, "_my_work")
		#thread_paint.start(self, "_my_loop")
		
	if $BrowserTexture.visible:
		$BrowserTexture.visible = false
	else:
		$BrowserTexture.visible = true
		$BrowserTexture.focus_next


func _on_BrowserTexture_gui_input(event):
	
	pass
	if b:
		if event is InputEventMouseButton:
			b.on_mouse_click(event.button_index, event.pressed)
			b.on_mouse_click(event.button_index, event.pressed)
		elif event is InputEventMouseMotion:
			b.on_mouse_moved(event.position.x, event.position.y)
			b.on_mouse_moved(event.position.x, event.position.y)
		elif event is InputEventKey:
			b.on_key_pressed(event.scancode, event.pressed)
			
			
###############################################################################


func _idle(delta):
	time_elapsed += delta
	# Return true to end the main loop.
	return quit

func _input_event(event):
	# Record keys.
	if event is InputEventKey and event.pressed and !event.echo:
		keys_typed.append(OS.get_scancode_string(event.scancode))
		# Quit on Escape press.
		if event.scancode == KEY_ESCAPE:
			quit = true

func _finalize():
	print("Stopping CEF ...")	
	_my_cef_stop()
	print("Finalized:")
	print("  End time: %s" % str(time_elapsed))
	print("  Keys typed: %s" % var2str(keys_typed))

func _my_cef_start():
	
	print("Initialized:")
	print("  Starting time: %s" % str(time_elapsed))
	print("Starting CEF ...")
	cef.cef_start()
	
func _my_cef_stop():
	cef.cef_stop()	
