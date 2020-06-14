// Copyright 2019-2020 LexLiu. All Rights Reserved.
#include "IDetailCustomization.h"
#pragma once

/**
 * 
 */
class FUISpriteCustomization : public IDetailCustomization
{
public:
	FUISpriteCustomization();
	~FUISpriteCustomization();

	static TSharedRef<IDetailCustomization> MakeInstance();
	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
private:
	TWeakObjectPtr<class UUISprite> TargetScriptPtr;
	void ForceRefresh(IDetailLayoutBuilder* DetailBuilder);

};
