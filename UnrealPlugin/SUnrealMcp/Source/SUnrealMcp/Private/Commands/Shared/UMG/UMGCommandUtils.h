#pragma once

#include "Binding/DynamicPropertyPath.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Commands/Shared/Blueprint/BlueprintCommandUtils.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Layout/Visibility.h"
#include "Mcp/SUnrealMcpProtocol.h"
#include "Styling/SlateColor.h"
#include "WidgetBlueprint.h"

namespace SUnrealMcpUMGCommandUtils
{
    inline UWidgetBlueprint* LoadWidgetBlueprintAsset(const FString& WidgetPath)
    {
        return WidgetPath.IsEmpty() ? nullptr : LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
    }

    inline UClass* ResolveWidgetParentClassReference(const FString& ParentClassReference)
    {
        UClass* ParentClass = SUnrealMcpBlueprintCommandUtils::ResolveParentClassReference(ParentClassReference);
        return ParentClass != nullptr && ParentClass->IsChildOf(UUserWidget::StaticClass()) ? ParentClass : nullptr;
    }

    inline void EnsureWidgetVariableGuid(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget)
    {
        if (WidgetBlueprint == nullptr || Widget == nullptr)
        {
            return;
        }

        const FName WidgetName = Widget->GetFName();
        if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(WidgetName))
        {
            WidgetBlueprint->WidgetVariableNameToGuidMap.Add(WidgetName, FGuid::NewGuid());
        }
    }

    inline UCanvasPanel* EnsureRootCanvasPanel(UWidgetBlueprint* WidgetBlueprint)
    {
        if (WidgetBlueprint == nullptr || WidgetBlueprint->WidgetTree == nullptr)
        {
            return nullptr;
        }

        if (UCanvasPanel* ExistingCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget))
        {
            EnsureWidgetVariableGuid(WidgetBlueprint, ExistingCanvas);
            return ExistingCanvas;
        }

        UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        if (RootCanvas != nullptr)
        {
            RootCanvas->bIsVariable = false;
            WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
            EnsureWidgetVariableGuid(WidgetBlueprint, RootCanvas);
        }

        return RootCanvas;
    }

    inline void ApplyCanvasSlotLayout(UCanvasPanelSlot* CanvasSlot, const FVector2D& Position, const FVector2D& Size)
    {
        if (CanvasSlot == nullptr)
        {
            return;
        }

        CanvasSlot->SetAutoSize(false);
        CanvasSlot->SetPosition(Position);
        CanvasSlot->SetSize(Size);
    }

    inline TSharedPtr<FJsonObject> BuildWidgetBlueprintSummary(UWidgetBlueprint* WidgetBlueprint)
    {
        const TSharedRef<FJsonObject> WidgetObject = MakeShared<FJsonObject>();
        WidgetObject->SetStringField(TEXT("name"), WidgetBlueprint->GetName());
        WidgetObject->SetStringField(TEXT("path"), WidgetBlueprint->GetPathName());
        WidgetObject->SetStringField(
            TEXT("generatedClass"),
            WidgetBlueprint->GeneratedClass != nullptr ? WidgetBlueprint->GeneratedClass->GetPathName() : TEXT(""));
        return WidgetObject;
    }

    inline FName GetBindingTargetPropertyName(const FString& BindingType)
    {
        if (BindingType.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
        {
            return TEXT("TextDelegate");
        }

        if (BindingType.Equals(TEXT("Visibility"), ESearchCase::IgnoreCase))
        {
            return TEXT("VisibilityDelegate");
        }

        if (BindingType.EndsWith(TEXT("Delegate")))
        {
            return FName(*BindingType);
        }

        return FName(*(BindingType + TEXT("Delegate")));
    }

    inline UEdGraph* EnsureFunctionGraph(UWidgetBlueprint* WidgetBlueprint, const FString& FunctionName)
    {
        if (WidgetBlueprint == nullptr || FunctionName.IsEmpty())
        {
            return nullptr;
        }

        TArray<UEdGraph*> AllGraphs;
        WidgetBlueprint->GetAllGraphs(AllGraphs);
        for (UEdGraph* Graph : AllGraphs)
        {
            if (Graph != nullptr && Graph->GetFName().ToString().Equals(FunctionName, ESearchCase::IgnoreCase))
            {
                return Graph;
            }
        }

        UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
            WidgetBlueprint,
            FName(*FunctionName),
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());

        if (NewGraph != nullptr)
        {
            FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBlueprint, NewGraph, true, nullptr);
        }

        return NewGraph;
    }

    inline FObjectProperty* FindWidgetVariableProperty(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName)
    {
        if (WidgetBlueprint == nullptr || WidgetName.IsEmpty())
        {
            return nullptr;
        }

        if (WidgetBlueprint->SkeletonGeneratedClass != nullptr)
        {
            if (FObjectProperty* Property = FindFProperty<FObjectProperty>(WidgetBlueprint->SkeletonGeneratedClass, *WidgetName))
            {
                return Property;
            }
        }

        if (WidgetBlueprint->GeneratedClass != nullptr)
        {
            if (FObjectProperty* Property = FindFProperty<FObjectProperty>(WidgetBlueprint->GeneratedClass, *WidgetName))
            {
                return Property;
            }
        }

        return nullptr;
    }
}

