﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "Core/ActorComponent/UIDirectMeshRenderable.h"
#include "LGUI.h"
#include "Core/ActorComponent/LGUICanvas.h"
#include "Core/LGUIMesh/LGUIMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UUIDirectMeshRenderable::UUIDirectMeshRenderable(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bLocalVertexPositionChanged = true;
	uiRenderableType = EUIRenderableType::UIDirectMeshRenderable;
}

void UUIDirectMeshRenderable::BeginPlay()
{
	Super::BeginPlay();
	MarkCanvasUpdate();
	bLocalVertexPositionChanged = true;
}

void UUIDirectMeshRenderable::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}


#if WITH_EDITOR
void UUIDirectMeshRenderable::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UUIDirectMeshRenderable::OnUnregister()
{
	Super::OnUnregister();
}

void UUIDirectMeshRenderable::ApplyUIActiveState()
{
	Super::ApplyUIActiveState();
}

void UUIDirectMeshRenderable::OnRenderCanvasChanged(ULGUICanvas* OldCanvas, ULGUICanvas* NewCanvas)
{
	Super::OnRenderCanvasChanged(OldCanvas, NewCanvas);
}




void UUIDirectMeshRenderable::UpdateCachedData()
{
	cacheForThisUpdate_LocalVertexPositionChanged = bLocalVertexPositionChanged;
	bLocalVertexPositionChanged = false;
	Super::UpdateCachedData();
}
void UUIDirectMeshRenderable::UpdateCachedDataBeforeGeometry()
{
	if (bLocalVertexPositionChanged)cacheForThisUpdate_LocalVertexPositionChanged = true;
	Super::UpdateCachedDataBeforeGeometry();
}
void UUIDirectMeshRenderable::MarkAllDirtyRecursive()
{
	bLocalVertexPositionChanged = true;
	Super::MarkAllDirtyRecursive();
}



void UUIDirectMeshRenderable::WidthChanged()
{
	MarkVertexPositionDirty();
}
void UUIDirectMeshRenderable::HeightChanged()
{
	MarkVertexPositionDirty();
}
void UUIDirectMeshRenderable::PivotChanged()
{
	MarkVertexPositionDirty();
}

void UUIDirectMeshRenderable::MarkVertexPositionDirty()
{
	bLocalVertexPositionChanged = true;
	MarkCanvasUpdate();
}
void UUIDirectMeshRenderable::UpdateGeometry()
{
	if (GetIsUIActiveInHierarchy() == false)return;
	if (!CheckRenderCanvas())return;

	Super::UpdateGeometry();
	if (!drawcall.IsValid()//not add to render yet
		)
	{
		RenderCanvas->AddUIRenderable(this);
	}
}


TWeakPtr<FLGUIMeshSection> UUIDirectMeshRenderable::GetMeshSection()const
{
	return MeshSection;
}
TWeakObjectPtr<ULGUIMeshComponent> UUIDirectMeshRenderable::GetUIMesh()const
{
	return UIMesh;
}
void UUIDirectMeshRenderable::ClearMeshData()
{
	UIMesh.Reset();
	MeshSection.Reset();
}
void UUIDirectMeshRenderable::SetMeshData(TWeakObjectPtr<ULGUIMeshComponent> InUIMesh, TWeakPtr<FLGUIMeshSection> InMeshSection)
{
	UIMesh = InUIMesh;
	MeshSection = InMeshSection;
}

bool UUIDirectMeshRenderable::LineTraceUI(FHitResult& OutHit, const FVector& Start, const FVector& End)
{
	if (bRaycastComplex)
	{
		if (!bRaycastTarget)return false;
		if (!GetIsUIActiveInHierarchy())return false;
		if (!RenderCanvas.IsValid())return false;
		if (!GetOwner())return false;
		if (GetOwner()->GetRootComponent() != this)return false;//only root component can do line trace hit

		if (!MeshSection.IsValid())return false;

		auto inverseTf = GetComponentTransform().Inverse();
		auto localSpaceRayOrigin = inverseTf.TransformPosition(Start);
		auto localSpaceRayEnd = inverseTf.TransformPosition(End);

		//DrawDebugLine(this->GetWorld(), Start, End, FColor::Red, false, 5.0f);//just for test
		//check Line-Plane intersection first, then check Line-Triangle
		//start and end point must be different side of z plane
		if (FMath::Sign(localSpaceRayOrigin.Z) != FMath::Sign(localSpaceRayEnd.Z))
		{
			auto result = FMath::LinePlaneIntersection(localSpaceRayOrigin, localSpaceRayEnd, FVector::ZeroVector, FVector(0, 0, 1));
			//hit point inside rect area
			if (result.X > GetLocalSpaceLeft() && result.X < GetLocalSpaceRight() && result.Y > GetLocalSpaceBottom() && result.Y < GetLocalSpaceTop())
			{
				//triangle hit test
				auto& vertices = MeshSection.Pin()->vertices;
				auto& triangleIndices = MeshSection.Pin()->triangles;
				int triangleCount = triangleIndices.Num() / 3;
				int index = 0;
				for (int i = 0; i < triangleCount; i++)
				{
					auto point0 = (vertices[triangleIndices[index++]].Position);
					auto point1 = (vertices[triangleIndices[index++]].Position);
					auto point2 = (vertices[triangleIndices[index++]].Position);
					FVector hitPoint, hitNormal;
					if (FMath::SegmentTriangleIntersection(localSpaceRayOrigin, localSpaceRayEnd, point0, point1, point2, hitPoint, hitNormal))
					{
						OutHit.TraceStart = Start;
						OutHit.TraceEnd = End;
						OutHit.Actor = GetOwner();
						OutHit.Component = (UPrimitiveComponent*)this;//acturally this convert is incorrect, but I need this pointer
						OutHit.Location = GetComponentTransform().TransformPosition(hitPoint);
						OutHit.Normal = GetComponentTransform().TransformVector(hitNormal);
						OutHit.Normal.Normalize();
						OutHit.Distance = FVector::Distance(Start, OutHit.Location);
						OutHit.ImpactPoint = OutHit.Location;
						OutHit.ImpactNormal = OutHit.Normal;
						return true;
					}
				}
			}
		}
		return false;
	}
	else
	{
		return Super::LineTraceUI(OutHit, Start, End);
	}
}
