import unreal_engine as ue
from threading import Thread

#internal, don't call directly
def backgroundAction(args=None):
	#ue.log(args)
	action = args[0]
	if len(args) >1:
		callback = args[1]

	#call the blocking action
	action()

	#return the result if we have a callback
	if callback:
		ue.run_on_gt(callback)

#run function on a background thread, optional callback when complete on game thread
def run_on_bt(actionfunction, callback=None):
	t = Thread(target=backgroundAction, args=([actionfunction, callback],))
	t.start()
