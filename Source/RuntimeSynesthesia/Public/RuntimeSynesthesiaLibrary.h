// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RuntimeSynesthesiaLibrary.generated.h"


class UOnsetNRT;
/**
 * 
 */
UCLASS(BlueprintType, Category = "Runtime Synesthesia")
class RUNTIMESYNESTHESIA_API URuntimeSynesthesiaLibrary : public UObject
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable)
	static URuntimeSynesthesiaLibrary* CreateRuntimeSynesthesia();

	UFUNCTION(BlueprintCallable)
		UOnsetNRT* Analyse(USoundWave* soundWave);
};