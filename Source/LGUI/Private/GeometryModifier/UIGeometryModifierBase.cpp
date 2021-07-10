﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "GeometryModifier/UIGeometryModifierBase.h"
#include "LGUI.h"
#include "Core/ActorComponent/UIBatchGeometryRenderable.h"

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
UUIBatchGeometryRenderable* UUIGeometryModifierBase::GetRenderableUIItem()
{
	if(!IsValid(renderableUIItem))
	{
		if (auto actor = GetOwner())
		{
			renderableUIItem = GetOwner()->FindComponentByClass<UUIBatchGeometryRenderable>();
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
			if (auto uiRenderable = Cast<UUIBatchGeometryRenderable>(rootComp))
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

