// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaProxyWidget.h"
#include "Components/ContentWidget.h"
#include "LuaDelegate.h"
#include "LuaProxySlot.h"
#include "LuaState.h"
#include "Components/CanvasPanel.h"

ULuaState* ULuaProxyWidget::GetLuaState()
{
	return Cast<ULuaState>(GetOuter());
}

FLuaValue ULuaProxyWidget::LuaMetaMethodToString_Implementation()
{
	return FString::Printf(TEXT("LuaProxyWidget@%p"), this);
}

FLuaValue ULuaProxyWidget::LuaMetaMethodIndex_Implementation(const FString& Key)
{
	if (Key == "SetContent")
	{
		FLuaValue SetContentWidget = FLuaValue([this](TArray<FLuaValue> LuaArgs) -> FLuaValueOrError
		{
			if (!Widget->IsA<UContentWidget>())
			{
				return FString("SetContent can be called only on ContentWidget instances");
			}
			if (!LuaArgs.IsValidIndex(0) || !LuaArgs[0].Object || !LuaArgs[0].Object->IsA<ULuaProxyWidget>())
			{
				return FString("Expected first argument to be a widget");
			}

			UPanelSlot* Slot = Cast<UContentWidget>(Widget)->SetContent(
				Cast<ULuaProxyWidget>(LuaArgs[0].Object)->Widget);
			if (Slot)
			{
				ULuaProxySlot* NewProxySlot = NewObject<ULuaProxySlot>(GetLuaState());
				NewProxySlot->Slot = Slot;
				Proxies.Add(NewProxySlot);
				return FLuaValue(NewProxySlot);
			}

			return FLuaValue();
		});

		return SetContentWidget;
	}
	if (Key == "AddChild" || Key == "AddChildToCanvas")
	{
		FLuaValue SetContentWidget = FLuaValue([this, &Key](TArray<FLuaValue> LuaArgs) -> FLuaValueOrError
		{
			if (!Widget->IsA<UPanelWidget>())
			{
				return FString::Printf(TEXT("%s can be called only on PanelWidget instances"), *Key);
			}
			if (!LuaArgs.IsValidIndex(0) || !LuaArgs[0].Object || !LuaArgs[0].Object->IsA<ULuaProxyWidget>())
			{
				return FString("Expected first argument to be a widget");
			}

			UPanelSlot* Slot = Cast<UPanelWidget>(Widget)->AddChild(Cast<ULuaProxyWidget>(LuaArgs[0].Object)->Widget);
			if (Slot)
			{
				ULuaProxySlot* NewProxySlot = NewObject<ULuaProxySlot>(GetLuaState());
				NewProxySlot->Slot = Slot;
				Proxies.Add(NewProxySlot);
				return FLuaValue(NewProxySlot);
			}
			return FLuaValue();
		});

		return SetContentWidget;
	}
	if (IsKnownProperty(Key))
	{
		return GetLuaState()->GetLuaValueFromProperty(Widget, *Key);
	}

	return FLuaValue();
}

bool ULuaProxyWidget::LuaMetaMethodNewIndex_Implementation(const FString& Key, FLuaValue Value)
{
	bool bSuccess = false;
	if (Key.StartsWith("On"))
	{
		bSuccess = GetLuaState()->SetPropertyFromLuaValue(Widget, *Key, Value);
	}
	else if (IsKnownProperty(Key))
	{
		bSuccess = GetLuaState()->SetPropertyFromLuaValue(Widget, *Key, Value);
	}

	if (bSuccess)
	{
		Widget->SynchronizeProperties();
	}

	return bSuccess;
}

bool ULuaProxyWidget::IsKnownProperty(const FString& Key)
{
	static const TSet<FName> KnownProperties = {
		"ColorAndOpacity",
		"Text",
		"CheckedState",
		"BrushColor",
		"Brush",
		"Justification",
	};

	return KnownProperties.Contains(FName(Key));
}
