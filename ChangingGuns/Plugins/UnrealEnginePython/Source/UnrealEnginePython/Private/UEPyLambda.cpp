#include "UnrealEnginePythonPrivatePCH.h"

PyObject * py_ue_run_on_game_thread(PyObject* PyFunction, PyObject* Args)
{
	//Todo: convert from input to function

	//Dispatch function on game thread
	FFunctionGraphTask::CreateAndDispatchWhenReady([PyFunction, Args]
	{
		//Call the PyFunction with Args here after acquiring GIL
	}, TStatId(), nullptr, ENamedThreads::GameThread);
	
	return Py_False;
}

