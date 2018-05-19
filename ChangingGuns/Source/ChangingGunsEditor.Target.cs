// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ChangingGunsEditorTarget : TargetRules
{
	public ChangingGunsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "ChangingGuns" } );
	}
}
