﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#include "GeometryModifier/UIGeometryModifierBase.h"
#include "LGUI.h"
#include "Core/ActorComponent/UIRenderable.h"

UUIGeometryModifierBase::UUIGeometryModifierBase()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUIGeometryModifierBase::BeginPlay()
{
	Super::BeginPlay();
	
}
void UUIGeometryModifierBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
}
UUIRenderable*& UUIGeometryModifierBase::GetRenderableUIItem()
{
	if(!IsValid(renderableUIItem))
	{
		if (auto actor = GetOwner())
		{
			renderableUIItem = GetOwner()->FindComponentByClass<UUIRenderable>();
		}
	}
	return renderableUIItem;
}
#if WITH_EDITOR
void UUIGeometryModifierBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (GetOwner())
	{
		if (auto rootComp = GetOwner()->GetRootComponent())
		{
			if (auto uiRenderable = Cast<UUIRenderable>(rootComp))
			{
				uiRenderable->EditorForceUpdateImmediately();
			}
		}
	}
}
#endif

void UUIGeometryModifierBase::OnRegister()
{
	Super::OnRegister();
	if (IsValid(GetRenderableUIItem()))
	{
		renderableUIItem->AddGeometryModifier(this);
		renderableUIItem->MarkTriangleDirty();
	}
}
void UUIGeometryModifierBase::OnUnregister()
{
	Super::OnUnregister();
	if (IsValid(GetRenderableUIItem()))
	{
		renderableUIItem->RemoveGeometryModifier(this);
		renderableUIItem->MarkTriangleDirty();
	}
}

void UUIGeometryModifierBase::ModifyUIGeometry(TSharedPtr<UIGeometry>& InGeometry, int32& InOutOriginVerticesCount, int32& InOutOriginTriangleIndicesCount, bool& OutTriangleChanged)
{
	UE_LOG(LGUI, Warning, TEXT("The function \"UUIGeometryModifierBase/ModifyUIGeometry\" must be override by a child class!"));
}
