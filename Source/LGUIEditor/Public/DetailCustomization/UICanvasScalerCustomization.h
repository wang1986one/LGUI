// Copyright 2019-2021 LexLiu. All Rights Reserved.
#include "IDetailCustomization.h"
#pragma once

/**
 * 
 */
class FUICanvasScalerCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance();
	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
private:
	TWeakObjectPtr<class ULGUICanvasScaler> TargetScriptPtr;
	float GetMatchValue()const;
	TOptional<float> GetMatchValueOptional()const;
	void SetMatchValue(float value, bool fromSlider);
	TSharedPtr<SHorizontalBox> ValueBox;
	FOptionalSize GetValueWidth()const;
};
