// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesTextFields.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesTextFieldsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Live;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Committed;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString NotesLive;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString NotesCommitted;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 CommitCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetLive(const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Commit(const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetNotesLive(const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void CommitNotes(const FString& Value);
};

namespace ArcRmlSamplesTextFields
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
