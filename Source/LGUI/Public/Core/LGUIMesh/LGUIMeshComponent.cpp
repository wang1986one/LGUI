﻿// Copyright 2019 LexLiu. All Rights Reserved.

#include "LGUIMeshComponent.h"
#include "DynamicMeshBuilder.h"
#include "PhysicsEngine/BodySetup.h"
#include "Containers/ResourceArray.h"
#include "StaticMeshResources.h"
#include "Materials/Material.h"
#include "Core/Render/ILGUIHudPrimitive.h"
#include "Core/Render/LGUIRenderer.h"
#include "Engine.h"
#include "LGUI.h"


/** Class representing a single section of the LGUI mesh */
class FLGUIMeshProxySection
{
public:
	/** Material applied to this section */
	UMaterialInterface* Material;
	/** Vertex buffer for this section */
	FStaticMeshVertexBuffers VertexBuffers;
	/** Index buffer for this section */
	FDynamicMeshIndexBuffer16 IndexBuffer;
	/** Vertex factory for this section */
	FLocalVertexFactory VertexFactory;
	/** Whether this section is currently visible */
	bool bSectionVisible;

	FLGUIMeshProxySection(ERHIFeatureLevel::Type InFeatureLevel)
		: Material(NULL)
		, VertexFactory(InFeatureLevel, "FLGUIMeshProxySection")
		, bSectionVisible(true)
	{}
};

DECLARE_CYCLE_STAT(TEXT("UpdateMeshSection_RenderThread"), STAT_UpdateMeshSectionRT, STATGROUP_LGUI);
/** LGUI mesh scene proxy */
class FLGUIMeshSceneProxy : public FPrimitiveSceneProxy, public ILGUIHudPrimitive
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
	FLGUIMeshSceneProxy(ULGUIMeshComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, BodySetup(InComponent->GetBodySetup())
		, MaterialRelevance(InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, RenderPriority(InComponent->TranslucencySortPriority)
	{
		FLGUIMeshSection& SrcSection = InComponent->MeshSection;
		if (SrcSection.vertices.Num() > 0)
		{
			FLGUIMeshProxySection* NewSection = new FLGUIMeshProxySection(GetScene().GetFeatureLevel());
			// vertex and index buffer
			NewSection->IndexBuffer.Indices = SrcSection.triangles;

			NewSection->VertexBuffers.InitFromDynamicVertex(&NewSection->VertexFactory, SrcSection.vertices, 4);

			// Enqueue initialization of render resource
			BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
			BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
			BeginInitResource(&NewSection->VertexBuffers.ColorVertexBuffer);
			BeginInitResource(&NewSection->IndexBuffer);
			BeginInitResource(&NewSection->VertexFactory);

			// Grab material
			NewSection->Material = InComponent->GetMaterial(0);
			if (NewSection->Material == NULL)
			{
				NewSection->Material = UMaterial::GetDefaultMaterial(MD_Surface);
			}

			// Copy visibility info
			NewSection->bSectionVisible = SrcSection.bSectionVisible;

			// Save ref to new section
			Section = NewSection;
		}
		LGUIHudRenderer = InComponent->LGUIHudRenderer;
		if (LGUIHudRenderer.IsValid())
		{
			LGUIHudRenderer.Pin()->AddHudPrimitive(this);
			IsHudOrWorldSpace = true;
		}
		else
		{
			IsHudOrWorldSpace = false;
		}
	}

	virtual ~FLGUIMeshSceneProxy()
	{
		if (Section != nullptr)
		{
			Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
			Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
			Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
			Section->IndexBuffer.ReleaseResource();
			Section->VertexFactory.ReleaseResource();
			delete Section;
		}
		if (LGUIHudRenderer.IsValid())
		{
			LGUIHudRenderer.Pin()->RemoveHudPrimitive(this);
		}
	}

	/** Called on render thread to assign new dynamic data */
	void UpdateSection_RenderThread(FDynamicMeshVertex* MeshVertexData, int32 NumVerts, uint16* MeshIndexData, uint32 IndexDataLength, int8 AdditionalChannelFlags)
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateMeshSectionRT);

		check(IsInRenderingThread());

		// Check it references a valid section
		if (Section != nullptr)
		{
			{
				if (AdditionalChannelFlags == 0)
				{
					for (int i = 0; i < NumVerts; i++)
					{
						const FDynamicMeshVertex& LGUIVert = MeshVertexData[i];
						Section->VertexBuffers.PositionVertexBuffer.VertexPosition(i) = LGUIVert.Position;
						Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, LGUIVert.TextureCoordinate[0]);
						Section->VertexBuffers.ColorVertexBuffer.VertexColor(i) = LGUIVert.Color;
					}
				}
				else
				{
					bool requireNormal = (AdditionalChannelFlags & (1 << 0)) != 0;
					bool requireTangent = (AdditionalChannelFlags & (1 << 1)) != 0;
					bool requireNormalOrTangent = requireNormal || requireTangent;
					bool requireUV1 = (AdditionalChannelFlags & (1 << 2)) != 0;
					bool requireUV2 = (AdditionalChannelFlags & (1 << 3)) != 0;
					bool requireUV3 = (AdditionalChannelFlags & (1 << 4)) != 0;
					for (int i = 0; i < NumVerts; i++)
					{
						const FDynamicMeshVertex& LGUIVert = MeshVertexData[i];
						Section->VertexBuffers.PositionVertexBuffer.VertexPosition(i) = LGUIVert.Position;
						Section->VertexBuffers.ColorVertexBuffer.VertexColor(i) = LGUIVert.Color;
						if (requireNormalOrTangent)
							Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, LGUIVert.TangentX.ToFVector(), LGUIVert.GetTangentY(), LGUIVert.TangentZ.ToFVector());
						Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, LGUIVert.TextureCoordinate[0]);
						if (requireUV1)
							Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 1, LGUIVert.TextureCoordinate[1]);
						if (requireUV2)
							Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 2, LGUIVert.TextureCoordinate[2]);
						if (requireUV3)
							Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 3, LGUIVert.TextureCoordinate[3]);
					}
				}
			}

			{
				auto& VertexBuffer = Section->VertexBuffers.PositionVertexBuffer;
				void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
				FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
				RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
			}

			{
				auto& VertexBuffer = Section->VertexBuffers.ColorVertexBuffer;
				void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
				FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
				RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
			}

			{
				auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
				void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
				FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
				RHIUnlockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
			}

			{
				auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
				void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
				FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
				RHIUnlockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
			}



			// Lock index buffer
			auto IndexBufferData = RHILockIndexBuffer(Section->IndexBuffer.IndexBufferRHI, 0, IndexDataLength, RLM_WriteOnly);
			FMemory::Memcpy(IndexBufferData, (void*)MeshIndexData, IndexDataLength);
			RHIUnlockIndexBuffer(Section->IndexBuffer.IndexBufferRHI);
		}
		delete[]MeshVertexData;
		delete[]MeshIndexData;
	}

	void SetSectionVisibility_RenderThread(bool bNewVisibility)
	{
		check(IsInRenderingThread());

		if (Section != nullptr)
		{
			Section->bSectionVisible = bNewVisibility;
		}
	}
	void SetToHud_RenderThread()
	{
		IsHudOrWorldSpace = true;
	}
	void SetToWorld_RenderThread()
	{
		IsHudOrWorldSpace = false;
	}
	void SetRenderPriority_RenderThread(int32 NewPriority)
	{
		RenderPriority = NewPriority;
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		//SCOPE_CYCLE_COUNTER(STAT_LGUIMesh_GetMeshElements);
		if (IsHudOrWorldSpace)return;
		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
				FLinearColor(0, 0.5f, 1.f)
			);

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		if (Section != nullptr && Section->bSectionVisible)
		{
			FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Section->Material->GetRenderProxy(IsSelected());

			// For each view..
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					// Draw the mesh.
					FMeshBatch& Mesh = Collector.AllocateMesh();
					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.IndexBuffer = &Section->IndexBuffer;
					Mesh.bWireframe = bWireframe;
					Mesh.VertexFactory = &Section->VertexFactory;
					Mesh.MaterialRenderProxy = MaterialProxy;
					BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
					BatchElement.FirstIndex = 0;
					BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
					Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
					Mesh.Type = PT_TriangleList;
					Mesh.DepthPriorityGroup = SDPG_World;
					Mesh.bCanApplyViewModeOverrides = false;
					Collector.AddMesh(ViewIndex, Mesh);
				}
			}
		}
	}

	//begin ILGUIHudPrimitive interface
	virtual FMeshBatch GetMeshElement() override
	{
		//if (Section != nullptr && Section->bSectionVisible)//check CanRender before call GetMeshElement, so this line is not necessary
		if (IsHudOrWorldSpace)
		{
			FMaterialRenderProxy* MaterialProxy = Section->Material->GetRenderProxy(false);

			// Draw the mesh.
			FMeshBatch Mesh;
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.IndexBuffer = &Section->IndexBuffer;
			Mesh.bWireframe = false;
			Mesh.VertexFactory = &Section->VertexFactory;
			Mesh.MaterialRenderProxy = MaterialProxy;
			BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.Type = PT_TriangleList;
			Mesh.DepthPriorityGroup = SDPG_World;
			Mesh.bCanApplyViewModeOverrides = false;
			return Mesh;
		}
		return FMeshBatch();
	}
	virtual int GetRenderPriority()const override
	{
		return RenderPriority;
	}
	virtual bool CanRender()const override
	{
		return Section != nullptr && Section->bSectionVisible;
	}
	//end ILGUIHudPrimitive interface

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}

private:
	FLGUIMeshProxySection* Section;
	UBodySetup* BodySetup;

	FMaterialRelevance MaterialRelevance;
	int32 RenderPriority = 0;
	TWeakPtr<FLGUIViewExtension, ESPMode::ThreadSafe> LGUIHudRenderer;
	bool IsHudOrWorldSpace = false;
};

//////////////////////////////////////////////////////////////////////////

void ULGUIMeshComponent::CreateMeshSection()
{
	UpdateLocalBounds(); // Update overall bounds
	MarkRenderStateDirty(); // New section requires recreating scene proxy
}

DECLARE_CYCLE_STAT(TEXT("UpdateMeshSection"), STAT_UpdateMeshSection, STATGROUP_LGUI);
void ULGUIMeshComponent::UpdateMeshSection(bool InVertexPositionChanged, int8 AdditionalShaderChannelFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateMeshSection);
	if (InVertexPositionChanged)
	{
		UpdateLocalBounds();
	}
	if (SceneProxy)
	{
		const int32 NumVerts = MeshSection.vertices.Num();
		FDynamicMeshVertex* VertexBufferData = new FDynamicMeshVertex[NumVerts];
		FMemory::Memcpy(VertexBufferData, MeshSection.vertices.GetData(), NumVerts * sizeof(FDynamicMeshVertex));
		//index data
		auto& triangles = MeshSection.triangles;
		const int32 NumTriangles = triangles.Num();
		uint16* IndexBufferData = new uint16[NumTriangles];
		uint32 IndexDataLength = NumTriangles * sizeof(uint16);
		FMemory::Memcpy(IndexBufferData, triangles.GetData(), IndexDataLength);
		ENQUEUE_UNIQUE_RENDER_COMMAND_SIXPARAMETER(
			FLGUIMeshUpdate,
			FLGUIMeshSceneProxy*, LGUIMeshSceneProxy, (FLGUIMeshSceneProxy*)SceneProxy,
			FDynamicMeshVertex*, VertexBufferData, VertexBufferData,
			int32, NumVerts, NumVerts,
			uint16*, IndexBufferData, IndexBufferData,
			uint32, IndexDataLength, IndexDataLength,
			int8, AdditionalShaderChannelFlags, AdditionalShaderChannelFlags,
			{
				LGUIMeshSceneProxy->UpdateSection_RenderThread(VertexBufferData, NumVerts, IndexBufferData, IndexDataLength, AdditionalShaderChannelFlags);
			}
		)
	}
}

void ULGUIMeshComponent::ClearMesh()
{
	MeshSection.Reset();
	UpdateLocalBounds();
	MarkRenderStateDirty();
}

void ULGUIMeshComponent::SetMeshVisible(bool bNewVisibility)
{
	this->SetVisibility(bNewVisibility);
	// Set game thread state
	if (MeshSection.bSectionVisible == bNewVisibility)return;
	MeshSection.bSectionVisible = bNewVisibility;

	if (SceneProxy)
	{
		// Enqueue command to modify render thread info
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FLGUIMeshVisibilityUpdate,
			FLGUIMeshSceneProxy*, LGUIMeshSceneProxy, (FLGUIMeshSceneProxy*)SceneProxy,
			bool, bNewVisibility, bNewVisibility,
			{
				LGUIMeshSceneProxy->SetSectionVisibility_RenderThread(bNewVisibility);
			}
		);
	}
}

bool ULGUIMeshComponent::IsMeshVisible() const
{
	return MeshSection.bSectionVisible;
}

void ULGUIMeshComponent::SetUITranslucentSortPriority(int32 NewTranslucentSortPriority)
{
	UPrimitiveComponent::SetTranslucentSortPriority(NewTranslucentSortPriority);
	if (SceneProxy)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FLGUIMesh_SetUITranslucentSortPriority,
			FLGUIMeshSceneProxy*, LGUIMeshSceneProxy, (FLGUIMeshSceneProxy*)SceneProxy,
			int32, NewRenderPriority, NewTranslucentSortPriority,
			{
				LGUIMeshSceneProxy->SetRenderPriority_RenderThread(NewRenderPriority);
			}
		)
	}
}

void ULGUIMeshComponent::UpdateLocalBounds()
{
	UpdateBounds();// Update global bounds
	// Need to send to render thread
	MarkRenderTransformDirty();
}

FPrimitiveSceneProxy* ULGUIMeshComponent::CreateSceneProxy()
{
	//SCOPE_CYCLE_COUNTER(STAT_LGUIMesh_CreateSceneProxy);

	FLGUIMeshSceneProxy* Proxy = NULL;
	if (MeshSection.vertices.Num() > 0)
	{
		Proxy = new FLGUIMeshSceneProxy(this);
	}
	return Proxy;
}

void ULGUIMeshComponent::SetToLGUIHud(TWeakPtr<FLGUIViewExtension, ESPMode::ThreadSafe> HudRenderer)
{
	LGUIHudRenderer = HudRenderer;
	if (SceneProxy)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			LGUIMeshComponent_SetToLGUIHud,
			FLGUIMeshSceneProxy*, LGUIMeshSceneProxy, (FLGUIMeshSceneProxy*)SceneProxy,
			{
				LGUIMeshSceneProxy->SetToHud_RenderThread();
			}
		);
	}
}

void ULGUIMeshComponent::SetToLGUIWorld()
{
	LGUIHudRenderer.Reset();
	if (SceneProxy)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			LGUIMeshComponent_SetToLGUIWorld,
			FLGUIMeshSceneProxy*, LGUIMeshSceneProxy, (FLGUIMeshSceneProxy*)SceneProxy,
			{
				LGUIMeshSceneProxy->SetToWorld_RenderThread();
			}
		);
	}
}

int32 ULGUIMeshComponent::GetNumMaterials() const
{
	return 1;
}

FBoxSphereBounds ULGUIMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const auto& vertices = MeshSection.vertices;
	int vertCount = vertices.Num();
	if (vertCount < 2)return Super::CalcBounds(LocalToWorld);

	FVector vecMin = vertices[0].Position;
	FVector vecMax = vecMin;

	// Get maximum and minimum X, Y and Z positions of vectors
	for (int32 i = 1; i < vertCount; i++)
	{
		auto vertPos = vertices[i].Position;
		vecMin.X = (vecMin.X > vertPos.X) ? vertPos.X : vecMin.X;
		vecMin.Y = (vecMin.Y > vertPos.Y) ? vertPos.Y : vecMin.Y;
		vecMin.Z = (vecMin.Z > vertPos.Z) ? vertPos.Z : vecMin.Z;

		vecMax.X = (vecMax.X < vertPos.X) ? vertPos.X : vecMax.X;
		vecMax.Y = (vecMax.Y < vertPos.Y) ? vertPos.Y : vecMax.Y;
		vecMax.Z = (vecMax.Z < vertPos.Z) ? vertPos.Z : vecMax.Z;
	}

	FVector vecOrigin = ((vecMax - vecMin) / 2) + vecMin;	/* Origin = ((Max Vertex's Vector - Min Vertex's Vector) / 2 ) + Min Vertex's Vector */
	FVector BoxPoint = vecMax - vecMin;			/* The difference between the "Maximum Vertex" and the "Minimum Vertex" is our actual Bounds Box */

	return FBoxSphereBounds(vecOrigin, BoxPoint, BoxPoint.Size()).TransformBy(LocalToWorld);
}

void ULGUIMeshComponent::SetColor(FColor InColor)
{
	auto& vertices = MeshSection.vertices;
	int count = vertices.Num();
	for (int i = 0; i < count; i++)
	{
		vertices[i].Color = InColor;
	}
	UpdateMeshSection(false);
}
FColor ULGUIMeshComponent::GetColor()const
{
	if (MeshSection.vertices.Num() > 0)
	{
		return MeshSection.vertices[0].Color;
	}
	else
	{
		return FColor::White;
	}
}