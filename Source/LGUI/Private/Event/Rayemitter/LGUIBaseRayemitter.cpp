﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#include "Event/Rayemitter/LGUIBaseRayEmitter.h"
#include "LGUI.h"


ULGUIBaseRayEmitter::ULGUIBaseRayEmitter()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}
void ULGUIBaseRayEmitter::BeginPlay()
{
	Super::BeginPlay();
}
void ULGUIBaseRayEmitter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool ULGUIBaseRayEmitter::EmitRay(FVector& OutRayOrigin, FVector& OutRayDirection, TArray<AActor*>& InOutTraceOnlyActors, TArray<AActor*>& InOutTraceIgnoreActors)
{
	UE_LOG(LGUI, Error, TEXT("[ULGUIBaseRayEmitter]Function EmitRay must be implemented!"));
	return false;
}

bool ULGUIBaseRayEmitter::ShouldStartDrag(ULGUIPointerEventData* InPointerEventData)
{
	UE_LOG(LGUI, Error, TEXT("[ULGUIBaseRayEmitter]Function ShouldStartDrag must be implemented!"));
	return false;
}
