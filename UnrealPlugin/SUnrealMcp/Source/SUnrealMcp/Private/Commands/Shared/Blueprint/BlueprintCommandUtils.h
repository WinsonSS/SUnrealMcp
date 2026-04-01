#pragma once

#include "Commands/Shared/Editor/EditorCommandUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/UObjectIterator.h"

namespace SUnrealMcpBlueprintCommandUtils
{
    inline UBlueprint* LoadBlueprintAsset(const FString& BlueprintPath)
    {
        return BlueprintPath.IsEmpty() ? nullptr : LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    }

    inline bool SplitAssetPath(const FString& AssetPath, FString& OutPackagePath, FString& OutAssetName)
    {
        if (AssetPath.IsEmpty())
        {
            return false;
        }

        const int32 DotIndex = AssetPath.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        const FString ObjectPath = DotIndex != INDEX_NONE ? AssetPath.Left(DotIndex) : AssetPath;
        const int32 SlashIndex = ObjectPath.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (SlashIndex == INDEX_NONE || SlashIndex <= 0 || SlashIndex >= ObjectPath.Len() - 1)
        {
            return false;
        }

        OutPackagePath = ObjectPath.Left(SlashIndex);
        OutAssetName = ObjectPath.Mid(SlashIndex + 1);
        return !OutPackagePath.IsEmpty() && !OutAssetName.IsEmpty();
    }

    inline UClass* ResolveParentClassReference(const FString& ParentClassReference)
    {
        if (ParentClassReference.IsEmpty())
        {
            return AActor::StaticClass();
        }

        if (UClass* ClassByPath = LoadObject<UClass>(nullptr, *ParentClassReference))
        {
            return ClassByPath;
        }

        FString CanonicalName = ParentClassReference;
        if (!CanonicalName.StartsWith(TEXT("U")) && !CanonicalName.StartsWith(TEXT("A")))
        {
            if (ParentClassReference.Equals(TEXT("Actor"), ESearchCase::IgnoreCase)
                || ParentClassReference.Equals(TEXT("Pawn"), ESearchCase::IgnoreCase)
                || ParentClassReference.Equals(TEXT("Character"), ESearchCase::IgnoreCase))
            {
                CanonicalName = TEXT("A") + ParentClassReference;
            }
            else
            {
                CanonicalName = TEXT("U") + ParentClassReference;
            }
        }

        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;
            if (Class == nullptr)
            {
                continue;
            }

            if (Class->GetName().Equals(CanonicalName, ESearchCase::IgnoreCase)
                || Class->GetName().Equals(ParentClassReference, ESearchCase::IgnoreCase)
                || Class->GetPathName().Equals(ParentClassReference, ESearchCase::IgnoreCase))
            {
                return Class;
            }
        }

        return nullptr;
    }

    inline UClass* ResolveComponentClassReference(const FString& ComponentType)
    {
        if (ComponentType.IsEmpty())
        {
            return nullptr;
        }

        if (UClass* ClassByPath = LoadObject<UClass>(nullptr, *ComponentType))
        {
            return ClassByPath->IsChildOf(UActorComponent::StaticClass()) ? ClassByPath : nullptr;
        }

        TArray<FString> CandidateNames;
        CandidateNames.Add(ComponentType);

        if (!ComponentType.StartsWith(TEXT("U")))
        {
            CandidateNames.Add(TEXT("U") + ComponentType);
        }

        if (!ComponentType.EndsWith(TEXT("Component")))
        {
            CandidateNames.Add(ComponentType + TEXT("Component"));
            if (!ComponentType.StartsWith(TEXT("U")))
            {
                CandidateNames.Add(TEXT("U") + ComponentType + TEXT("Component"));
            }
        }

        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;
            if (Class == nullptr || !Class->IsChildOf(UActorComponent::StaticClass()))
            {
                continue;
            }

            for (const FString& CandidateName : CandidateNames)
            {
                if (Class->GetName().Equals(CandidateName, ESearchCase::IgnoreCase)
                    || Class->GetPathName().Equals(ComponentType, ESearchCase::IgnoreCase))
                {
                    return Class;
                }
            }
        }

        return nullptr;
    }

    inline USCS_Node* FindBlueprintComponentNode(UBlueprint* Blueprint, const FString& ComponentName)
    {
        if (Blueprint == nullptr || Blueprint->SimpleConstructionScript == nullptr || ComponentName.IsEmpty())
        {
            return nullptr;
        }

        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node != nullptr && Node->GetVariableName().ToString().Equals(ComponentName, ESearchCase::IgnoreCase))
            {
                return Node;
            }
        }

        return nullptr;
    }

    inline TSharedPtr<FJsonObject> BuildBlueprintSummary(UBlueprint* Blueprint)
    {
        const TSharedRef<FJsonObject> BlueprintObject = MakeShared<FJsonObject>();
        BlueprintObject->SetStringField(TEXT("name"), Blueprint->GetName());
        BlueprintObject->SetStringField(TEXT("path"), Blueprint->GetPathName());
        BlueprintObject->SetStringField(
            TEXT("generatedClass"),
            Blueprint->GeneratedClass != nullptr ? Blueprint->GeneratedClass->GetPathName() : TEXT(""));
        BlueprintObject->SetBoolField(TEXT("isDataOnly"), FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint));
        return BlueprintObject;
    }
}

