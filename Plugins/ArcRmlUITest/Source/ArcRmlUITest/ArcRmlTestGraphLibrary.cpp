// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlTestGraphLibrary.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "K2Node_CallFunction.h"
#include "K2Node_GetSubsystem.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Subsystems/WorldSubsystem.h"

namespace ArcRmlTestGraphLibraryPrivate
{
	UClass* GetterNodeClass(const UClass* SubsystemClass)
	{
		if (SubsystemClass->IsChildOf<ULocalPlayerSubsystem>())
		{
			return FindObject<UClass>(FTopLevelAssetPath(TEXT("/Script/BlueprintGraph"), TEXT("K2Node_GetSubsystemFromPC")));
		}
		if (SubsystemClass->IsChildOf<UGameInstanceSubsystem>() || SubsystemClass->IsChildOf<UWorldSubsystem>())
		{
			return UK2Node_GetSubsystem::StaticClass();
		}
		return nullptr;
	}
}

UK2Node* UArcRmlTestGraphLibrary::AddCallFunctionNode(UEdGraph* Graph, UClass* FunctionClass, FName FunctionName, FIntPoint Position)
{
	const UFunction* Function = (Graph && FunctionClass) ? FunctionClass->FindFunctionByName(FunctionName) : nullptr;
	if (!Function)
	{
		return nullptr;
	}
	return FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_CallFunction>(Graph, FVector2D(Position), EK2NewNodeFlags::None,
		[Function](UK2Node_CallFunction* NewNode)
		{
			NewNode->SetFromFunction(Function);
		});
}

UK2Node* UArcRmlTestGraphLibrary::FindCallFunctionNode(UEdGraph* Graph, UClass* FunctionClass, FName FunctionName)
{
	const UFunction* Function = (Graph && FunctionClass) ? FunctionClass->FindFunctionByName(FunctionName) : nullptr;
	if (!Function)
	{
		return nullptr;
	}
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node);
		if (CallNode && CallNode->GetTargetFunction() == Function)
		{
			return CallNode;
		}
	}
	return nullptr;
}

UK2Node* UArcRmlTestGraphLibrary::AddSubsystemGetterNode(UEdGraph* Graph, TSubclassOf<USubsystem> SubsystemClass, FIntPoint Position)
{
	UClass* NodeClass = (Graph && SubsystemClass) ? ArcRmlTestGraphLibraryPrivate::GetterNodeClass(SubsystemClass) : nullptr;
	if (!NodeClass)
	{
		return nullptr;
	}
	UClass* Subsystem = SubsystemClass;
	UEdGraphNode* NewNode = FEdGraphSchemaAction_K2NewNode::CreateNode(Graph, TArrayView<UEdGraphPin*>(), FVector2D(Position),
		[NodeClass](UEdGraph* ParentGraph) -> UK2Node*
		{
			return NewObject<UK2Node>(ParentGraph, NodeClass);
		},
		[Subsystem](UK2Node* Node)
		{
			CastChecked<UK2Node_GetSubsystem>(Node)->Initialize(Subsystem);
		},
		EK2NewNodeFlags::None);
	return Cast<UK2Node>(NewNode);
}

bool UArcRmlTestGraphLibrary::SplitPin(UK2Node* Node, FName PinName)
{
	UEdGraphPin* Pin = Node ? Node->FindPin(PinName, EGPD_Input) : nullptr;
	if (!Pin)
	{
		return false;
	}
	if (Pin->SubPins.IsEmpty())
	{
		GetDefault<UEdGraphSchema_K2>()->SplitPin(Pin);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(FBlueprintEditorUtils::FindBlueprintForNodeChecked(Node));
	}
	return !Pin->SubPins.IsEmpty();
}
