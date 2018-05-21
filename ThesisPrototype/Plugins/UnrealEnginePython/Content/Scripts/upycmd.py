#ue.exec('pip2.py')

import subprocess
import sys
import os
import unreal_engine as ue
import _thread as thread

#ue.log(sys.path)
_problemPaths = ['']

def NormalizePaths():
	problemPaths = _problemPaths

	#replace '/' to '\\'
	for i in range(len(sys.path)):
		currentPath = sys.path[i]
		sys.path[i] = currentPath.replace('\\','/')

		#find additional problem paths such as engine bin
		currentPath = sys.path[i]
		if('Engine' in currentPath and 'Epic Games' in currentPath):
			_problemPaths.append(currentPath)

	#cleanup
	for path in problemPaths:
		if path in sys.path:
			sys.path.remove(path)

#define some convenience paths
def PythonHomePath():
	for path in sys.path:
		normalizedPath = AsAbsPath(path)
		if ('UnrealEnginePython' in normalizedPath and
			normalizedPath.endswith('Binaries/Win64')):
			return path
	
	#return sys.path[1]
	return "not found"

def PythonHomeScriptsPath():
	return AsAbsPath(PythonHomePath() + "/Scripts")

def PythonPluginScriptPath():
	for path in sys.path:
		normalizedPath = AsAbsPath(path)
		if ('UnrealEnginePython' in normalizedPath and
			normalizedPath.endswith('Content/Scripts')):
			return path

	return "not found"

def PythonProjectScriptPath():
	relativePath = PythonPluginScriptPath() + "/../../../../Content/Scripts";
	return AsAbsPath(relativePath);

def AsAbsPath(path):
	return os.path.abspath(path).replace('\\','/')

_PythonHomePath = PythonHomePath()

def FolderCommand(folder):
	#replace backslashes
	folder = folder.replace('/','\\')

	changefolder = "cd /d \"" + folder + "\" & "
	return changefolder

#main public function
def run(process, path=_PythonHomePath, verbose=True):
	#todo: change folder
	fullcommand = FolderCommand(path) + process
	if verbose:
		ue.log("Started cmd <" + fullcommand + ">")
	stdoutdata = subprocess.getstatusoutput(fullcommand)
	if verbose:
		ue.log("cmd Result: ")
		ue.log(stdoutdata[1])
	return stdoutdata[1] #return the data for dependent functions

def runStreaming(process, callback=None, path=_PythonHomePath, verbose=True):
	#todo: change folder
	fullcommand = FolderCommand(path) + process

	if verbose:
		print("Started cmd <" + fullcommand + ">")

	#streaming version
	popenobj = subprocess.Popen(fullcommand, stdout=subprocess.PIPE)
	output = ''
	for line in iter(process.stdout.readline, ''):
		#sys.stdout.write(line)
		print(line)
		output += line


	if verbose:
		print("cmd Result: ")
		print(output)
	return output #return the data for dependent functions

#convenience override
def runLogOutput(process, path=_PythonHomePath):
	fullcommand = FolderCommand(path) + process
	stdoutdata = subprocess.getstatusoutput(fullcommand)
	ue.log(stdoutdata[1])
	return stdoutdata[1]

#convenience wrappers
def dir(path=_PythonHomePath):
	run('dir', path)

def ls(path=_PythonHomePath):
	dir(path)

def md(folder, path=_PythonHomePath):
	run('md ' + folder, path)

def mkdir(folder, path=_PythonHomePath):
	md(folder, path)