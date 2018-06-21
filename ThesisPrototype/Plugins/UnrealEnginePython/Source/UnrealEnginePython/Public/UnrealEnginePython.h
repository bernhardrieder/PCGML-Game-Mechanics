// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif

#include "ModuleManager.h"
#define UEPY_THREADING 1

#include "Engine.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Runtime/SlateCore/Public/Styling/ISlateStyle.h"
#include "Runtime/SlateCore/Public/Styling/SlateStyle.h"

// We need to make sure reference structs do not mistaken for callable
#define PyCalllable_Check_Extended(value) PyCallable_Check(value) && py_ue_is_uscriptstruct(value) == nullptr

#if ENGINE_MINOR_VERSION >= 18
#define FStringAssetReference FSoftObjectPath
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogPython, Log, All);


class UNREALENGINEPYTHON_API FUnrealEnginePythonModule : public IModuleInterface
{
public:

	bool PythonGILAcquire();
	void PythonGILRelease();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RunString(char *);
	void RunStringSandboxed(char *);
	void RunFile(char *);
	void RunFileSandboxed(char *, void(*callback)(void *arg), void *arg);

	void UESetupPythonInterpreter(bool);

	FString ScriptsPath;
	FString ZipPath;
	FString AdditionalModulesPath;

	bool BrutalFinalize;

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline FUnrealEnginePythonModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FUnrealEnginePythonModule >("UnrealEnginePython");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UnrealEnginePython");
	}

	//Add specified folder to the python systems path (e.g. from external plugins or projects allowing modular python plugin script content)
	void AddPathToSysPath(const FString& Path);

	void AddPythonDependentPlugin(const FString& PluginName);

	// pep8ize a string using various strategy (currently only autopep8 is supported)
	FString Pep8ize(FString Code);

private:
	void *ue_python_gil;
	// used by console
	void *main_dict;
	void *local_dict;
	void *main_module;

	TSharedPtr<FSlateStyleSet> StyleSet;
};



struct FScopePythonGIL
{
	FScopePythonGIL()
	{
#if defined(UEPY_THREADING)
		UnrealEnginePythonModule = FModuleManager::LoadModuleChecked<FUnrealEnginePythonModule>("UnrealEnginePython");
		safeForRelease = UnrealEnginePythonModule.PythonGILAcquire();
#endif
	}

	~FScopePythonGIL()
	{
#if defined(UEPY_THREADING)
		if (safeForRelease)
		{
			UnrealEnginePythonModule.PythonGILRelease();
		}
#endif
	}

	FUnrealEnginePythonModule UnrealEnginePythonModule;
	bool safeForRelease;
};




