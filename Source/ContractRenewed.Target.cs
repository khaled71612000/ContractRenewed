// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class ContractRenewedTarget : TargetRules
{
	public ContractRenewedTarget(TargetInfo Target) : base(Target)
	{
        Type = TargetType.Game;

        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        CppStandard = CppStandardVersion.Cpp20;
        WindowsPlatform.bStrictConformanceMode = true;
        bValidateFormatStrings = true;

        ExtraModuleNames.AddRange( new string[] { "ContractRenewed" } );
	}
}
