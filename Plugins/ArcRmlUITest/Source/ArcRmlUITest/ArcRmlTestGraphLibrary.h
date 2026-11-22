// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/Subsystem.h"
#include "Templates/SubclassOf.h"

#include "ArcRmlTestGraphLibrary.generated.h"

class UEdGraph;
class UK2Node;

UCLASS()
class UArcRmlTestGraphLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ArcRmlUI|Tests")
	static UK2Node* AddCallFunctionNode(UEdGraph* Graph, UClass* FunctionClass, FName FunctionName, FIntPoint Position);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlUI|Tests")
	static UK2Node* FindCallFunctionNode(UEdGraph* Graph, UClass* FunctionClass, FName FunctionName);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlUI|Tests")
	static UK2Node* AddSubsystemGetterNode(UEdGraph* Graph, TSubclassOf<USubsystem> SubsystemClass, FIntPoint Position);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlUI|Tests")
	static bool SplitPin(UK2Node* Node, FName PinName);
};
