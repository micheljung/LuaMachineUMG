// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaUserWidget.h"

#include "LuaBlueprintFunctionLibrary.h"
#include "LuaProxyWidget.h"
#include "Blueprint/WidgetTree.h"

FLuaValue ULuaUserWidget::LuaMetaMethodToString_Implementation()
{
	return FString::Printf(TEXT("LuaUserWidget@%p"), this);
}

FLuaValue ULuaUserWidget::LuaMetaMethodIndex_Implementation(const FString& Key)
{
	if (Key.StartsWith("Create"))
	{
		const FString WidgetClassName = Key.Mid(6);

		UClass* WidgetClass = FindFirstObject<UClass>(*WidgetClassName);
		if (!WidgetClass)
		{
			UE_LOG(LogLuaMachine, Error, TEXT("%s is an invalid UWidget class name"), *WidgetClassName);
			return FLuaValue();
		}

		if (!WidgetClass->IsChildOf<UWidget>())
		{
			UE_LOG(LogLuaMachine, Error, TEXT("%s is not a UWidget"), *WidgetClassName);
			return FLuaValue();
		}

		FLuaValue CreateNewWidget = FLuaValue([this, WidgetClass](TArray<FLuaValue>) -> FLuaValueOrError
		{
			UWidget* NewWidget = WidgetTree->ConstructWidget<UWidget>(WidgetClass);
			ULuaProxyWidget* NewProxyWidget = NewObject<ULuaProxyWidget>(OwningLuaState);
			NewProxyWidget->Widget = NewWidget;
			Proxies.Add(NewProxyWidget);
			return FLuaValue(NewProxyWidget);
		});

		return CreateNewWidget;
	}
	if (Key == "SetRoot")
	{
		FLuaValue SetRootWidget = FLuaValue([this](TArray<FLuaValue> LuaArgs) -> FLuaValueOrError
		{
			if (!LuaArgs.IsValidIndex(0) || !LuaArgs[0].Object || !LuaArgs[0].Object->IsA<ULuaProxyWidget>())
			{
				return FString("Expected first argument to be a widget");
			}

			if (WidgetTree->RootWidget)
			{
				RemoveFromParent();
			}
			WidgetTree->RootWidget = Cast<ULuaProxyWidget>(LuaArgs[0].Object)->Widget;
			return FLuaValue();
		});

		return SetRootWidget;
	}
	if (Key == "AddToViewport")
	{
		FLuaValue AddToViewportWidget = FLuaValue([this](TArray<FLuaValue> LuaArgs) -> FLuaValueOrError
		{
			AddToViewport();
			return FLuaValue();
		});

		return AddToViewportWidget;
	}

	return FLuaValue();
}

bool ULuaUserWidget::LuaMetaMethodNewIndex_Implementation(const FString& Key, FLuaValue Value)
{
	bool bSuccess = false;
	if (Key.StartsWith("OnMouseButtonDown"))
	{
		MouseButtonDownHandler.BindLambda([this, Value](const FGeometry& Geometry, const FPointerEvent& Event)
		{
			FLuaValue LocalSize = ULuaBlueprintFunctionLibrary::LuaTableFromFVector2F(
				OwningLuaState, OwningLuaState->GetClass(), Geometry.GetLocalSize());
			FLuaValue AbsoluteSize = ULuaBlueprintFunctionLibrary::LuaTableFromFVector2F(
				OwningLuaState, OwningLuaState->GetClass(), Geometry.GetAbsoluteSize());
			FLuaValue AbsolutePosition = ULuaBlueprintFunctionLibrary::LuaTableFromFVector2F(
				OwningLuaState, OwningLuaState->GetClass(), Geometry.GetAbsolutePosition());

			FLuaValue LuaGeometry = OwningLuaState->CreateLuaTable();
			LuaGeometry.SetField("LocalSize", LocalSize);
			LuaGeometry.SetField("AbsoluteSize", AbsoluteSize);
			LuaGeometry.SetField("AbsolutePosition", AbsolutePosition);

			FLuaValue CursorDelta = ULuaBlueprintFunctionLibrary::LuaTableFromFVector2F(
				OwningLuaState, OwningLuaState->GetClass(), Event.GetCursorDelta());
			FLuaValue ScreenSpacePosition = ULuaBlueprintFunctionLibrary::LuaTableFromFVector2F(
				OwningLuaState, OwningLuaState->GetClass(), Event.GetScreenSpacePosition());
			FLuaValue LastScreenSpacePosition = ULuaBlueprintFunctionLibrary::LuaTableFromFVector2F(
				OwningLuaState, OwningLuaState->GetClass(), Event.GetLastScreenSpacePosition());
			FLuaValue PointerIndex = static_cast<int32>(Event.GetPointerIndex());

			TArray<FLuaValue> PressedButtonsArray;
			for (FKey PressedButton : Event.GetPressedButtons())
			{
				PressedButtonsArray.Add(OwningLuaState->StructToLuaValue(PressedButton));
			}
			FLuaValue PressedButtons = ULuaBlueprintFunctionLibrary::LuaTablePack(
				OwningLuaState, OwningLuaState->GetClass(), PressedButtonsArray);

			FLuaValue LuaEvent = OwningLuaState->CreateLuaTable();
			LuaEvent.SetField("CursorDelta", CursorDelta);
			LuaEvent.SetField("ScreenSpacePosition", ScreenSpacePosition);
			LuaEvent.SetField("LastScreenSpacePosition", LastScreenSpacePosition);
			LuaEvent.SetField("PressedButtons", PressedButtons);
			LuaEvent.SetField("PointerIndex", PointerIndex);

			OwningLuaState->LuaValueCall(Value, {LuaGeometry, LuaEvent});

			return FReply::Handled();
		});
		bSuccess = true;
	}

	return bSuccess;
}

FReply ULuaUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (MouseButtonDownHandler.IsBound())
	{
		return MouseButtonDownHandler.Execute(InGeometry, InMouseEvent);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
