﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#pragma once

#include "UIBaseRenderable.h"
#include "UIPostProcess.generated.h"

class FUIPostProcessRenderProxy;
/** 
 * UI element that can add post processing effect
 * Only valid on ScreenSpaceUI
 */
UCLASS(Abstract, NotBlueprintable, Experimental)
class LGUI_API UUIPostProcess : public UUIBaseRenderable
{
	GENERATED_BODY()

public:	
	UUIPostProcess(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void OnUnregister()override;
	virtual void ApplyUIActiveState() override;
	TSharedPtr<UIGeometry> geometry = nullptr;
	virtual void UpdateGeometry(const bool& parentLayoutChanged)override final;

	virtual void OnRenderCanvasChanged(ULGUICanvas* OldCanvas, ULGUICanvas* NewCanvas)override;
	virtual void WidthChanged()override;
	virtual void HeightChanged()override;
	virtual void PivotChanged()override;

	virtual void UpdateBasePrevData()override;
	virtual void UpdateCachedData()override;
	virtual void UpdateCachedDataBeforeGeometry()override;
	virtual void MarkAllDirtyRecursive()override;

public:
	void MarkVertexPositionDirty();
	void MarkUVDirty();
	TSharedPtr<UIGeometry> GetGeometry() { return geometry; }
public:
	virtual TWeakPtr<FUIPostProcessRenderProxy> GetRenderProxy()PURE_VIRTUAL(UUIPostProcess::GetRenderProxy, return 0;);
private:
	/** local vertex position changed */
	uint8 bLocalVertexPositionChanged : 1;
	/** vertex's uv change */
	uint8 bUVChanged : 1;

	uint8 cacheForThisUpdate_LocalVertexPositionChanged : 1, cacheForThisUpdate_UVChanged : 1;
protected:
	TSharedPtr<FUIPostProcessRenderProxy> RenderProxy = nullptr;
	/** create ui geometry */
	virtual void OnCreateGeometry()PURE_VIRTUAL(UUIRenderable::OnCreateGeometry, );
	/** update ui geometry */
	virtual void OnUpdateGeometry(bool InVertexPositionChanged, bool InVertexUVChanged, bool InVertexColorChanged)PURE_VIRTUAL(UUIRenderable::OnUpdateGeometry, );
private:
	void CreateGeometry();
};
