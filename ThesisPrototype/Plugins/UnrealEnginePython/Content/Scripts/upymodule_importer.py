#Imports upymodule.json modules and resolves dependencies

#example test
#import upymodule_importer as upym
#upym.parseJson("D:/Users/Admin/Documents/GitHub/tensorflow-ue4-mnist-example/Plugins/tensorflow-ue4/Content/Scripts/upymodule.json")

import upypip as pip
import sys
import unreal_engine as ue
import json

import upypip
import upycmd
import imp
import site
pip = upypip

#parse upymodule.json from path and install dependencies if missing
def parseJson(packagePath):
	#we may have added paths, normalize them
	imp.reload(site)

	try:
		with open(packagePath) as data_file:
			#TODO: catch file not found error    
			package = json.load(data_file)
			ue.log('upymodule_importer::Resolving upymodule dependencies for ' + package['name'])
			pythonModules = package['pythonModules']

			#loop over all the modules, check if we have them installed
			for module in pythonModules:
				version = pythonModules[module]

				ue.log("upymodule_importer::" + module + " " + version + " installed? " + str(pip.isInstalled(module)))
				if not pip.isInstalled(module, version):
					ue.log('upymodule_importer::Dependency not installed, fetching via pip...')
					if version == 'latest':
						pip.install(module)
					else:
						pip.install(module + '==' + version)

			dependencyNoun = 'dependencies'
			if len(pythonModules) == 1:
				dependencyNoun = 'dependency'

			ue.log('upymodule_importer::' + str(len(pythonModules)) + ' ' + package['name'] + ' upymodule ' + dependencyNoun + ' resolved (if installation in progress, more async output will stream)')
	except:
		e = sys.exc_info()[0]
		ue.log('upymodule_importer::upymodule.json error: ' + str(e))

def containsModuleFile(packagePath):
	try:
		with open(packagePath) as data_file:
			return True
	except:
		return False

