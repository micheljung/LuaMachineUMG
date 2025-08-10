// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LuaUserDataInterface.h"
#include "LuaState.h"
#include "LuaUserWidget.generated.h"

DECLARE_DELEGATE_RetVal_TwoParams(FReply, FMouseButtonDownHandler, const FGeometry&, const FPointerEvent&);

UCLASS()
class LUAMACHINEUMG_API ULuaUserWidget : public UUserWidget, public ILuaUserDataInterface
{
	GENERATED_BODY()

public:
	FLuaValue LuaMetaMethodIndex_Implementation(const FString& Key) override;
	
	bool LuaMetaMethodNewIndex_Implementation(const FString& Key, FLuaValue Value) override;

	FLuaValue LuaMetaMethodToString_Implementation() override;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	UPROPERTY()
	ULuaState* OwningLuaState;

	UPROPERTY()
	TSet<class ULuaProxyWidget*> Proxies;
	
    FMouseButtonDownHandler MouseButtonDownHandler;
};
