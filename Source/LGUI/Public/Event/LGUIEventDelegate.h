﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#pragma once

#include "LGUIPointerEventData.h"
#include "LGUIDelegateHandleWrapper.h"
#include "LGUIDelegateDeclaration.h"
#include "LGUIEventDelegate.generated.h"


UENUM()
enum class LGUIEventDelegateParameterType :uint8
{
	/** not initialized */
	None		UMETA(Hidden),
	Empty,
	Bool		UMETA(DisplayName = "Boolean"),
	Float,
	Double		UMETA(Hidden),
	Int8		UMETA(Hidden),
	UInt8		UMETA(DisplayName = "UInt8\Enum\Byte"),
	Int16		UMETA(Hidden),
	UInt16		UMETA(Hidden),
	Int32		UMETA(DisplayName = "Integer"),
	UInt32		UMETA(Hidden),
	Int64		UMETA(Hidden),
	UInt64		UMETA(Hidden),
	Vector2		UMETA(DisplayName = "Vector2"),
	Vector3		UMETA(DisplayName = "Vector3"),
	Vector4		UMETA(DisplayName = "Vector4"),
	Color,
	LinearColor,
	Quaternion,
	String,
	/** for asset reference */
	Object,
	/** for actor reference in level */
	Actor,
	/** for LGUIPointerEventData */
	PointerEvent	UMETA(DisplayName = "LGUIPointerEventData"),
	/** Class for UClass reference */
	Class,
	
	Rotator,

	Name,
	Text,
};
/** helper class for finding function */
class LGUI_API ULGUIEventDelegateParameterHelper
{
public:
	static bool IsSupportedFunction(UFunction* Target, LGUIEventDelegateParameterType& OutParamType);
	static bool IsStillSupported(UFunction* Target, LGUIEventDelegateParameterType InParamType);
	static FString ParameterTypeToName(LGUIEventDelegateParameterType paramType, const UFunction* InFunction = nullptr);
	/** if first parameter is an object type, then return it's objectclass */
	static UClass* GetObjectParameterClass(const UFunction* InFunction);
	static UEnum* GetEnumParameter(const UFunction* InFunction);
	static UClass* GetClassParameterClass(const UFunction* InFunction);
private:
	static bool IsFunctionCompatible(const UFunction* InFunction, LGUIEventDelegateParameterType& OutParameterType);
	static bool IsPropertyCompatible(const FProperty* InFunctionProperty, LGUIEventDelegateParameterType& OutParameterType);
};

/**
 * Editable event type in editor
 */
USTRUCT()
struct LGUI_API FLGUIEventDelegateData
{
	GENERATED_BODY()
private:
	friend struct FLGUIEventDelegate;
	friend class FLGUIEventDelegateCustomization;
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")bool BoolValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")float FloatValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")double DoubleValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")int8 Int8Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")uint8 UInt8Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")int16 Int16Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")uint16 UInt16Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")int32 Int32Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")uint32 UInt32Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")int64 Int64Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")uint64 UInt64Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FVector2D Vector2Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FVector Vector3Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FVector4 Vector4Value;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FQuat QuatValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FColor ColorValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FLinearColor LinearColorValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FRotator RotatorValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FString StringValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FName NameValue;
	UPROPERTY(EditAnywhere, Transient, Category = "LGUI")FText TextValue;
#endif
#if WITH_EDITORONLY_DATA
	/** Editor helper actor, for direct reference actor */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		AActor* HelperActor;
	/** Editor helper, target object class. If class is actor then TargetObject is HelperActor, if class is ActorComponent then TargetObject is the component. */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		UClass* HelperClass;
	/** Editor helper, if TargetObject is actor component and HelperActor have multiple components, then select by component name. */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		FName HelperComponentName;
#endif
	UPROPERTY(EditAnywhere, Category = "LGUI")
		UObject* TargetObject;
	/** target function name */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		FName functionName;
	/** target function supported parameter type */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		LGUIEventDelegateParameterType ParamType = LGUIEventDelegateParameterType::None;

	/** data buffer stores function's parameter */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		TArray<uint8> ParamBuffer;
	/** Object reference, can reference actor/class/asset */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		UObject* ReferenceObject;

	/** use the function's native parameter? */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		bool UseNativeParameter = false;
private:
	UPROPERTY(Transient) UFunction* CacheFunction = nullptr;
	UPROPERTY(Transient) UObject* CacheTarget = nullptr;
public:
	void Execute();
	void Execute(void* InParam, LGUIEventDelegateParameterType InParameterType);
private:
	void FindAndExecute(UObject* Target, void* ParamData = nullptr);
	void ExecuteTargetFunction(UObject* Target, UFunction* Func);
	void ExecuteTargetFunction(UObject* Target, UFunction* Func, void* ParamData);
#if WITH_EDITORONLY_DATA
public:
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		AActor* targetActor;
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		UClass* componentClass;
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		FName componentName;
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		FName ReferenceName;
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		FString ReferenceString;
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		FText ReferenceText;
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		AActor* ReferenceActor;
	/** old data */
	UPROPERTY(VisibleAnywhere, Category = "LGUI-old")
		UClass* ReferenceClass;
	
#endif
};

/**
 * event or callback that can edit inside ue4 editor
 */
USTRUCT(BlueprintType)
struct LGUI_API FLGUIEventDelegate
{
	GENERATED_BODY()

public:
	FLGUIEventDelegate();
	FLGUIEventDelegate(LGUIEventDelegateParameterType InParameterType);
private:
	friend class FLGUIEventDelegateCustomization;
	/** event list */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		TArray<FLGUIEventDelegateData> eventList;
	/** supported parameter type of this event */
	UPROPERTY(EditAnywhere, Category = "LGUI", meta = (DisplayName = "NativeParameterType"))
		LGUIEventDelegateParameterType supportParameterType = LGUIEventDelegateParameterType::Empty;
	/** Parameter type must be the same as your declaration of FLGUIEventDelegate(LGUIEventDelegateParameterType InParameterType) */
	void FireEvent(void* InParam)const;
	void LogParameterError()const;
public:
	bool IsBound()const;
public:
	void FireEvent()const;
	void FireEvent(bool InParam)const;
	void FireEvent(float InParam)const;
	void FireEvent(double InParam)const;
	void FireEvent(int8 InParam)const;
	void FireEvent(uint8 InParam)const;
	void FireEvent(int16 InParam)const;
	void FireEvent(uint16 InParam)const;
	void FireEvent(int32 InParam)const;
	void FireEvent(uint32 InParam)const;
	void FireEvent(int64 InParam)const;
	void FireEvent(uint64 InParam)const;
	void FireEvent(FVector2D InParam)const;
	void FireEvent(FVector InParam)const;
	void FireEvent(FVector4 InParam)const;
	void FireEvent(FColor InParam)const;
	void FireEvent(FLinearColor InParam)const;
	void FireEvent(FQuat InParam)const; 
	void FireEvent(const FString& InParam)const;
	void FireEvent(UObject* InParam)const;
	void FireEvent(AActor* InParam)const;
	void FireEvent(ULGUIPointerEventData* InParam)const;
	void FireEvent(FRotator InParam)const;
	void FireEvent(const FName& InParam)const;
	void FireEvent(const FText& InParam)const;
};
