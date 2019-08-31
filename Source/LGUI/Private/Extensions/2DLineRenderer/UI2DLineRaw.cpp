﻿// Copyright 2019 LexLiu. All Rights Reserved.

#include "Extensions/2DLineRenderer/UI2DLineRaw.h"
#include "LGUI.h"
#include "Core/UIGeometry.h"
#include "Core/ActorComponent/LGUICanvas.h"

UUI2DLineRaw::UUI2DLineRaw()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUI2DLineRaw::BeginPlay()
{
	CurrentPointArray = PointArray;
	Super::BeginPlay();
}
#if WITH_EDITOR
void UUI2DLineRaw::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	CurrentPointArray = PointArray;
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

AUI2DLineActor::AUI2DLineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIElement = CreateDefaultSubobject<UUI2DLineRaw>(TEXT("UIElement"));
	RootComponent = UIElement;
}


void UUI2DLineRaw::OnCreateGeometry()
{
	CurrentPointArray = PointArray;
	Super::OnCreateGeometry();
}
void UUI2DLineRaw::OnUpdateGeometry(bool InVertexPositionChanged, bool InVertexUVChanged, bool InVertexColorChanged)
{
	Super::OnUpdateGeometry(InVertexPositionChanged, InVertexUVChanged, InVertexColorChanged);
}

void UUI2DLineRaw::SetPoints(const TArray<FVector2D>& InPoints)
{
	if (InPoints.Num() != PointArray.Num())
	{
		PointArray = InPoints;
		CurrentPointArray = PointArray;
		MarkTriangleDirty();
	}
	else
	{
		PointArray = InPoints;
		CurrentPointArray = PointArray;
		MarkVertexPositionDirty();
	}
}
