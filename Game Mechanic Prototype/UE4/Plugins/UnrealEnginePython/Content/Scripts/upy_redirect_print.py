import sys
import unreal_engine as ue

class Redirector(object):
    def __init__(self):
        pass

    def write(self, message):
        ue.log(message)

    def flush(self):
    	sys.stdout.flush()

    def splitlines(self):
    	sys.stdout.splitlines()

sys.stdout = Redirector()
sys.stderr = Redirector()