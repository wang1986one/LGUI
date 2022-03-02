﻿// Copyright 2019-2022 LexLiu. All Rights Reserved.

#include "GeometryModifier/UIEffectShadow.h"
#include "LGUI.h"


UUIEffectShadow::UUIEffectShadow()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UUIEffectShadow::ModifyUIGeometry(
	UIGeometry& InGeometry, bool InTriangleChanged, bool InUVChanged, bool InColorChanged, bool InVertexPositionChanged
)
{
	auto& triangles = InGeometry.triangles;
	auto& originPositions = InGeometry.originPositions;
	auto& vertices = InGeometry.vertices;

	auto vertexCount = originPositions.Num();
	int32 triangleCount = triangles.Num();
	if (triangleCount == 0 || vertexCount == 0)return;
	
	const int32 singleChannelTriangleIndicesCount = triangleCount;
	const int32 singleChannelVerticesCount = vertexCount;
	//create additional triangle pass
	triangles.AddUninitialized(singleChannelTriangleIndicesCount);
	//put orgin triangles on last pass, this will make the origin triangle render at top
	for (int i = singleChannelTriangleIndicesCount, j = 0; j < singleChannelTriangleIndicesCount; i++, j++)
	{
		auto index = triangles[j];
		triangles[i] = index;
		triangles[j] = index + singleChannelVerticesCount;
	}
	
	vertexCount = singleChannelVerticesCount + singleChannelVerticesCount;
	originPositions.Reserve(vertexCount);
	vertices.Reserve(vertexCount);
	for (int i = singleChannelVerticesCount; i < vertexCount; i++)
	{
		originPositions.Add(FVector());
		vertices.Add(FVector());
	}

	for (int channelIndex1 = singleChannelVerticesCount, channelIndexOrigin = 0; channelIndex1 < vertexCount; channelIndex1++, channelIndexOrigin++)
	{
		auto originVertPos = originPositions[channelIndexOrigin];
		originVertPos.Y += shadowOffset.X;
		originVertPos.Z += shadowOffset.Y;
		originPositions[channelIndex1] = originVertPos;

		if (multiplySourceAlpha)
		{
			auto& vertColor = vertices[channelIndex1].Color;
			vertColor.A = (uint8)(UUIBaseRenderable::Color255To1_Table[vertices[channelIndexOrigin].Color.A] * shadowColor.A);
			vertColor.R = shadowColor.R;
			vertColor.G = shadowColor.G;
			vertColor.B = shadowColor.B;
		}
		else
		{
			vertices[channelIndex1].Color = shadowColor;
		}

		vertices[channelIndex1].TextureCoordinate[0] = vertices[channelIndexOrigin].TextureCoordinate[0];
	}
}

void UUIEffectShadow::SetShadowColor(FColor newColor)
{
	if (shadowColor != newColor)
	{
		shadowColor = newColor;
		if (GetUIRenderable())GetUIRenderable()->MarkColorDirty();
	}
}
void UUIEffectShadow::SetShadowOffset(FVector2D newOffset)
{
	if (shadowOffset != newOffset)
	{
		shadowOffset = newOffset;
		if (GetUIRenderable())GetUIRenderable()->MarkVertexPositionDirty();
	}
}