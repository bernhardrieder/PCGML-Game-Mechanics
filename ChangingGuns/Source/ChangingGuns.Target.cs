// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ChangingGunsTarget : TargetRules
{
	public ChangingGunsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "ChangingGuns" } );
	}
}
