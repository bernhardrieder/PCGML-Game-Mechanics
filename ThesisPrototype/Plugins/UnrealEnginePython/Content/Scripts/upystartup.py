import sys
import unreal_engine as ue
import json
import upycmd as cmd
import os
from os import listdir
from os import path as ospath
import imp
import site
import upymodule_importer as upymod

import upypip as pip

def checkPipDirectory():
	#get our python scripts path
	configPath = cmd.AsAbsPath(cmd.PythonPluginScriptPath() + '/upyconfig.json')
	correctPipPath = cmd.PythonHomeScriptsPath()

	#check that we have a config file, if not make an empty one
	if not (os.path.exists(configPath)):
		with open(configPath, "w+") as configFile:
			configs = {}
			configs['pipDirectoryPath'] = ""
			configFile.seek(0)
			configFile.write(json.dumps(configs))

	#compare our current pip directory with the installed one, if they differ reinstall pip
	
	with open(configPath, "r+") as configFile:
		configs = json.load(configFile)

		#grab currently stored path
		storedPipPath = configs['pipDirectoryPath']

		print('upystartup::Checking pip location on startup')
		print('upystartup::stored loc: ' + storedPipPath)
		print('upystartup::correct loc: ' + correctPipPath)

		libPath = cmd.PythonHomePath() + '/Lib/site-packages'

		#compare paths
		if (storedPipPath != correctPipPath or
			not ospath.exists(libPath)):
		
			#if they don't match, remove the pip module and reinstall pip for this module
			if (ospath.exists(libPath)):
				print('upystartup::Pip installation directory is stale, re-installing.')
				dirs = listdir(libPath)

				tempPath = None
				#find the directory that contains pip and ends with .dist-info
				for directory in dirs:
					#print(directory)
					if (directory.startswith('pip') and
						directory.endswith('.dist-info')):
						tempPath = libPath + "/" + directory
						break

				if(tempPath != None):
					#remove the old directory
					print('removing old: ' + tempPath)
					cmd.run('rmdir /S /Q "' + tempPath + '"')

			else:
				#site path doesn't even exist
				print("Lib/site-packages missing, re-installing pip.")
			
			#install pip
			print(cmd.PythonHomePath() + '/get-pip.py')

			print('Installing pip...')
			cmd.runLogOutput('InstallPip.bat')

			#update our stored location
			configs['pipDirectoryPath'] = correctPipPath
			configFile.seek(0)
			configFile.write(json.dumps(configs))
			configFile.truncate()

			#reload site, this will also normalize paths
			imp.reload(site)

			#done
			print('upystartup::updated pip.exe location in <' + configPath + '>')

		else:
			print('upystartup::pip location is up to date.')

	#for all cases ensure we have normalized paths
	print('upystartup::system paths normalized.')
	cmd.NormalizePaths();

def checkProjectModuleFile():
	path = cmd.PythonProjectScriptPath() + '/upymodule.json'
	if upymod.containsModuleFile(path):
		upymod.parseJson(path)
		print("upystartup::project upymodule.json file parsed.")
	else:
		print("upystartup::project doesn't use a upymodule.json file.")

#add any startup action you wish to perform in python
def startup():
	#check that our pip directory matches for pip.exe commands
	checkPipDirectory()

	#import any project specific modules
	checkProjectModuleFile()
