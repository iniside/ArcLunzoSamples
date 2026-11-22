// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ArcRmlDemoAssets.generated.h"

class UBlueprint;
class UUserDefinedStruct;

UCLASS()
class UArcRmlDemoAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ArcRmlUIDemo|Editor")
	static bool SetVariableFieldNotify(UBlueprint* Blueprint, FName VariableName, bool bFieldNotify);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlUIDemo|Editor")
	static bool SetClassDefaultsFromJson(UClass* Class, const FString& Json);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlUIDemo|Editor")
	static UUserDefinedStruct* EnsureUserDefinedStruct(const FString& AssetPath, const TArray<FName>& MemberNames, const TArray<FEdGraphPinType>& MemberTypes);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlUIDemo|Editor")
	static int32 RemoveMemberVariablesNotIn(UBlueprint* Blueprint, const TArray<FName>& Keep);
};
