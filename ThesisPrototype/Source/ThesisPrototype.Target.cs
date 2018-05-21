// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ThesisPrototypeTarget : TargetRules
{
	public ThesisPrototypeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "ThesisPrototype" } );
	}
}
