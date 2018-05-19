#import debugtest
#import imp
#imp.reload(debugtest)

#Testing script for various writing setups

#import redirect_print
import unreal_engine as ue
import time
import sys
import upythread as ut
from threading import Thread

#imp.reload(redirect_print)

def onfinished(args=""):
	ue.log(args)
	ue.log('finished with: <' + str(args) + '>')

def onfinishedempty():
	ue.log('finished')

def testaction(args=""):
	ue.log('starting action with <' + str(args) + '>')
	#onfinished()

	#pretend you take time to finish
	time.sleep(1)
	ue.log('wait complete')
	ue.run_on_gt(onfinished, args)
	ue.run_on_gt(onfinishedempty)

#the test function
def test(params=None):
	ue.log(type(params))
	ue.log('starting test')
	if not params:
		t = Thread(target=testaction)
	else:
		t = Thread(target=testaction, args=(params,))
	t.start()

def yolo():
	ue.log('yolo!')

def yolodone():
	ue.log('yolo done!')

#test simple fire and forget
def test2():
	ut.run_on_bt(yolo)

#Test with callback
def test3():
	ut.run_on_bt(yolo, yolodone)


#progress callback example functions
def progresscallback(progress):
	ue.log('at ' + str(progress))

def doLongTask():
	ue.log('started my task')

	for x in range(1,10):
		time.sleep(0.5)
		ue.run_on_gt(progresscallback, x)

#test basic progress bar
def testp():
	ut.run_on_bt(doLongTask)