// Copyright 2019-2020 LexLiu. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "LGUISpriteFactory.generated.h"

UCLASS()
class ULGUISpriteFactory : public UFactory
{
	GENERATED_BODY()
public:
	ULGUISpriteFactory();

	class UTexture2D* SpriteTexture = nullptr;
	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};
