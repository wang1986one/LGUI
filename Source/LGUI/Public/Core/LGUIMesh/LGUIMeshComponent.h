// Copyright 2019-2021 LexLiu. All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "Core/LGUIIndexBuffer.h"
#include "DynamicMeshBuilder.h"
#include "LGUIMeshComponent.generated.h"

class FLGUIMeshProxySection;
struct FLGUIMeshSection
{
	TArray<FLGUIIndexType> triangles;
	TArray<FDynamicMeshVertex> vertices;

	int prevVertexCount = 0;
	int prevIndexCount = 0;

	FLGUIMeshProxySection* renderProxy = nullptr;
	UMaterialInterface* material = nullptr;

	bool bSectionVisible = true;
	int renderPriority = 0;

	FLGUIMeshSection(){}

	void Reset()
	{
		vertices.Reset();
		triangles.Reset();

		bSectionVisible = true;
	}
};

class FLGUIHudRenderer;
class ILGUIHudPrimitive;
class ULGUICanvas;

//Generate dynamic mesh
UCLASS(ClassGroup = (LGUI), NotBlueprintable)
class LGUI_API ULGUIMeshComponent : public UMeshComponent
{
	GENERATED_BODY()

public:
	void CreateMeshSection(TSharedPtr<FLGUIMeshSection> InMeshSection);
	void UpdateMeshSection(TSharedPtr<FLGUIMeshSection> InMeshSection, bool InVertexPositionChanged, int8 AdditionalShaderChannelFlags);
	void DeleteMeshSection(TSharedPtr<FLGUIMeshSection> InMeshSection);
	void ClearAllMeshSection();
	TSharedPtr<FLGUIMeshSection> GetMeshSection();
	void SetMeshSectionRenderPriority(TSharedPtr<FLGUIMeshSection> InMeshSection, int32 InSortPriority);
	void SortMeshSectionRenderPriority();
	void SetMeshSectionMaterial(TSharedPtr<FLGUIMeshSection> InMeshSection, UMaterialInterface* InMaterial);

	void SetMeshSectionVisibility(bool bNewVisibility, int InSectionIndex);
	bool IsMeshSectionVisible(int InSectionIndex) const;

	void SetSupportLGUIRenderer(bool supportOrNot, TWeakPtr<FLGUIHudRenderer, ESPMode::ThreadSafe> HudRenderer, ULGUICanvas* InCanvas, bool InIsRenderToWorld);
	void SetSupportUERenderer(bool supportOrNot);

	void SetUITranslucentSortPriority(int32 NewTranslucentSortPriority);

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

private:
	TArray<TSharedPtr<FLGUIMeshSection>> MeshSections;
	TArray<TSharedPtr<FLGUIMeshSection>> PooledMeshSections;
	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

	/** Update LocalBounds member from the local box of each section */
	void UpdateLocalBounds();

	virtual void DestroyRenderState_Concurrent()override;

	friend class FLGUIMeshSceneProxy;

protected:
	TWeakPtr<FLGUIHudRenderer, ESPMode::ThreadSafe> LGUIRenderer;
	bool IsLGUIRenderToWorld = false;//LGUI renderer render to world or screen
	TWeakObjectPtr<ULGUICanvas> RenderCanvas = nullptr;
	bool IsSupportUERenderer = true;
};


