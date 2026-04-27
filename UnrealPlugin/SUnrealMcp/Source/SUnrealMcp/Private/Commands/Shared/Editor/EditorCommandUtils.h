#pragma once

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Internationalization/Text.h"
#include "Mcp/SUnrealMcpProtocol.h"
#include "UObject/NameTypes.h"
#include "UObject/TextProperty.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace SUnrealMcpEditorCommandUtils
{
    inline UWorld* GetEditorWorld(const FString& RequestId, FSUnrealMcpResponse& OutErrorResponse)
    {
        if (GEditor == nullptr)
        {
            OutErrorResponse = FSUnrealMcpResponse::MakeError(
                RequestId,
                TEXT("EDITOR_UNAVAILABLE"),
                TEXT("GEditor is not available."));
            return nullptr;
        }

        UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
        if (EditorWorld == nullptr)
        {
            OutErrorResponse = FSUnrealMcpResponse::MakeError(
                RequestId,
                TEXT("WORLD_UNAVAILABLE"),
                TEXT("Editor world is not available."));
            return nullptr;
        }

        return EditorWorld;
    }

    inline TSharedPtr<FJsonObject> BuildActorSummary(AActor* Actor)
    {
        const TSharedRef<FJsonObject> ActorObject = MakeShared<FJsonObject>();
        ActorObject->SetStringField(TEXT("label"), Actor->GetActorLabel());
        ActorObject->SetStringField(TEXT("path"), Actor->GetPathName());
        ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetPathName());
        ActorObject->SetStringField(TEXT("level"), Actor->GetLevel() ? Actor->GetLevel()->GetPathName() : TEXT(""));

        const FVector Location = Actor->GetActorLocation();
        ActorObject->SetArrayField(TEXT("location"), {
            MakeShared<FJsonValueNumber>(Location.X),
            MakeShared<FJsonValueNumber>(Location.Y),
            MakeShared<FJsonValueNumber>(Location.Z),
        });

        const FRotator Rotation = Actor->GetActorRotation();
        ActorObject->SetArrayField(TEXT("rotation"), {
            MakeShared<FJsonValueNumber>(Rotation.Pitch),
            MakeShared<FJsonValueNumber>(Rotation.Yaw),
            MakeShared<FJsonValueNumber>(Rotation.Roll),
        });

        const FVector Scale = Actor->GetActorScale3D();
        ActorObject->SetArrayField(TEXT("scale"), {
            MakeShared<FJsonValueNumber>(Scale.X),
            MakeShared<FJsonValueNumber>(Scale.Y),
            MakeShared<FJsonValueNumber>(Scale.Z),
        });

        return ActorObject;
    }

    inline bool TryReadVector3(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, FVector& OutVector)
    {
        const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, JsonArray) || JsonArray->Num() != 3)
        {
            return false;
        }

        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;
        if (!(*JsonArray)[0]->TryGetNumber(X) || !(*JsonArray)[1]->TryGetNumber(Y) || !(*JsonArray)[2]->TryGetNumber(Z))
        {
            return false;
        }

        OutVector = FVector(X, Y, Z);
        return true;
    }

    inline bool UsesWildcardPattern(const FString& Pattern)
    {
        return Pattern.Contains(TEXT("*")) || Pattern.Contains(TEXT("?"));
    }

    inline bool MatchesActorLabel(const FString& Label, const FString& Pattern)
    {
        if (UsesWildcardPattern(Pattern))
        {
            return Label.MatchesWildcard(Pattern, ESearchCase::IgnoreCase);
        }

        return Label.Contains(Pattern, ESearchCase::IgnoreCase);
    }

    inline AActor* FindActorByPath(UWorld* EditorWorld, const FString& ActorPath)
    {
        if (EditorWorld == nullptr || ActorPath.IsEmpty())
        {
            return nullptr;
        }

        if (AActor* DirectActor = FindObject<AActor>(nullptr, *ActorPath))
        {
            return DirectActor->GetWorld() == EditorWorld ? DirectActor : nullptr;
        }

        for (TActorIterator<AActor> It(EditorWorld); It; ++It)
        {
            AActor* Actor = *It;
            if (Actor != nullptr && Actor->GetPathName() == ActorPath)
            {
                return Actor;
            }
        }

        return nullptr;
    }

    inline FProperty* FindObjectProperty(UObject* Object, const FString& PropertyName)
    {
        if (Object == nullptr || PropertyName.IsEmpty())
        {
            return nullptr;
        }

        for (TFieldIterator<FProperty> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
        {
            FProperty* Property = *It;
            if (Property != nullptr && Property->GetName().Equals(PropertyName, ESearchCase::IgnoreCase))
            {
                return Property;
            }
        }

        return nullptr;
    }

    inline FProperty* FindActorProperty(AActor* Actor, const FString& PropertyName)
    {
        return Actor != nullptr ? FindObjectProperty(Actor, PropertyName) : nullptr;
    }

    inline bool IsEditableObjectProperty(FProperty* Property)
    {
        return Property != nullptr
            && !Property->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient)
            && !Property->GetName().StartsWith(TEXT("bHiddenEd"));
    }

    inline bool IsExposedActorProperty(FProperty* Property)
    {
        return IsEditableObjectProperty(Property);
    }

    inline UClass* ResolveActorClassPath(const FString& ActorClassReference)
    {
        if (ActorClassReference.IsEmpty())
        {
            return nullptr;
        }

        if (ActorClassReference.Contains(TEXT("/")) && ActorClassReference.Contains(TEXT(".")))
        {
            UClass* LoadedClass = LoadObject<UClass>(nullptr, *ActorClassReference);
            return LoadedClass != nullptr && LoadedClass->IsChildOf(AActor::StaticClass()) ? LoadedClass : nullptr;
        }

        TArray<FString> CandidateNames;
        CandidateNames.Add(ActorClassReference);
        if (!ActorClassReference.StartsWith(TEXT("A")))
        {
            CandidateNames.Add(TEXT("A") + ActorClassReference);
        }

        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;
            if (Class == nullptr || !Class->IsChildOf(AActor::StaticClass()))
            {
                continue;
            }

            for (const FString& CandidateName : CandidateNames)
            {
                if (Class->GetName().Equals(CandidateName, ESearchCase::IgnoreCase)
                    || Class->GetPathName().Equals(ActorClassReference, ESearchCase::IgnoreCase))
                {
                    return Class;
                }
            }
        }

        return nullptr;
    }

    inline UClass* LoadBlueprintActorClass(const FString& BlueprintPath, FString& OutError)
    {
        if (BlueprintPath.IsEmpty())
        {
            OutError = TEXT("Blueprint path is empty.");
            return nullptr;
        }

        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
        if (Blueprint == nullptr)
        {
            OutError = FString::Printf(TEXT("Could not load Blueprint asset '%s'."), *BlueprintPath);
            return nullptr;
        }

        UClass* GeneratedClass = Blueprint->GeneratedClass;
        if (GeneratedClass == nullptr || !GeneratedClass->IsChildOf(AActor::StaticClass()))
        {
            OutError = FString::Printf(
                TEXT("Blueprint '%s' does not have a generated Actor class."),
                *BlueprintPath);
            return nullptr;
        }

        return GeneratedClass;
    }

    inline TSharedPtr<FJsonValue> ExportPropertyValueToJson(FProperty* Property, const void* Container)
    {
        if (Property == nullptr || Container == nullptr)
        {
            return MakeShared<FJsonValueNull>();
        }

        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);
        if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
        {
            return MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(ValuePtr));
        }

        if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
        {
            if (NumericProperty->IsFloatingPoint())
            {
                return MakeShared<FJsonValueNumber>(NumericProperty->GetFloatingPointPropertyValue(ValuePtr));
            }

            return MakeShared<FJsonValueNumber>(static_cast<double>(NumericProperty->GetSignedIntPropertyValue(ValuePtr)));
        }

        if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
        {
            return MakeShared<FJsonValueString>(StringProperty->GetPropertyValue(ValuePtr));
        }

        if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
        {
            return MakeShared<FJsonValueString>(NameProperty->GetPropertyValue(ValuePtr).ToString());
        }

        if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
        {
            return MakeShared<FJsonValueString>(TextProperty->GetPropertyValue(ValuePtr).ToString());
        }

        if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            if (UObject* ObjectValue = ObjectProperty->GetObjectPropertyValue(ValuePtr))
            {
                return MakeShared<FJsonValueString>(ObjectValue->GetPathName());
            }

            return MakeShared<FJsonValueNull>();
        }

        if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            const int64 EnumValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
            if (const UEnum* Enum = EnumProperty->GetEnum())
            {
                return MakeShared<FJsonValueString>(Enum->GetNameStringByValue(EnumValue));
            }
        }

        if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
        {
            if (const UEnum* Enum = ByteProperty->Enum)
            {
                return MakeShared<FJsonValueString>(Enum->GetNameStringByValue(ByteProperty->GetPropertyValue(ValuePtr)));
            }
        }

        if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
        {
            if (StructProperty->Struct == TBaseStructure<FVector>::Get())
            {
                const FVector& VectorValue = *StructProperty->ContainerPtrToValuePtr<FVector>(Container);
                return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{
                    MakeShared<FJsonValueNumber>(VectorValue.X),
                    MakeShared<FJsonValueNumber>(VectorValue.Y),
                    MakeShared<FJsonValueNumber>(VectorValue.Z),
                });
            }

            if (StructProperty->Struct == TBaseStructure<FRotator>::Get())
            {
                const FRotator& RotatorValue = *StructProperty->ContainerPtrToValuePtr<FRotator>(Container);
                return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{
                    MakeShared<FJsonValueNumber>(RotatorValue.Pitch),
                    MakeShared<FJsonValueNumber>(RotatorValue.Yaw),
                    MakeShared<FJsonValueNumber>(RotatorValue.Roll),
                });
            }
        }

        FString ExportedText;
        Property->ExportText_InContainer(0, ExportedText, Container, Container, nullptr, PPF_None);
        return MakeShared<FJsonValueString>(ExportedText);
    }

    inline bool ApplyJsonValueToProperty(
        const TSharedPtr<FJsonValue>& JsonValue,
        FProperty* Property,
        void* Container,
        FString& OutError)
    {
        if (!JsonValue.IsValid() || Property == nullptr || Container == nullptr)
        {
            OutError = TEXT("Property value, property metadata, or target container is invalid.");
            return false;
        }

        void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Container);

        if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
        {
            bool BoolValue = false;
            if (!JsonValue->TryGetBool(BoolValue))
            {
                OutError = TEXT("Expected a boolean value.");
                return false;
            }

            BoolProperty->SetPropertyValue(ValuePtr, BoolValue);
            return true;
        }

        if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
        {
            double NumberValue = 0.0;
            if (!JsonValue->TryGetNumber(NumberValue))
            {
                OutError = TEXT("Expected a numeric value.");
                return false;
            }

            if (NumericProperty->IsFloatingPoint())
            {
                NumericProperty->SetFloatingPointPropertyValue(ValuePtr, NumberValue);
            }
            else
            {
                NumericProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(NumberValue));
            }
            return true;
        }

        if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
        {
            FString StringValue;
            if (!JsonValue->TryGetString(StringValue))
            {
                OutError = TEXT("Expected a string value.");
                return false;
            }

            StringProperty->SetPropertyValue(ValuePtr, StringValue);
            return true;
        }

        if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
        {
            FString StringValue;
            if (!JsonValue->TryGetString(StringValue))
            {
                OutError = TEXT("Expected a string value.");
                return false;
            }

            NameProperty->SetPropertyValue(ValuePtr, FName(*StringValue));
            return true;
        }

        if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
        {
            FString StringValue;
            if (!JsonValue->TryGetString(StringValue))
            {
                OutError = TEXT("Expected a string value.");
                return false;
            }

            TextProperty->SetPropertyValue(ValuePtr, FText::FromString(StringValue));
            return true;
        }

        if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            if (JsonValue->Type == EJson::Null)
            {
                ObjectProperty->SetObjectPropertyValue(ValuePtr, nullptr);
                return true;
            }

            FString ObjectPath;
            if (!JsonValue->TryGetString(ObjectPath))
            {
                OutError = TEXT("Expected a string object path or null.");
                return false;
            }

            UObject* LoadedObject = LoadObject<UObject>(nullptr, *ObjectPath);
            if (LoadedObject == nullptr)
            {
                OutError = FString::Printf(TEXT("Could not load object '%s'."), *ObjectPath);
                return false;
            }

            if (!LoadedObject->IsA(ObjectProperty->PropertyClass))
            {
                OutError = FString::Printf(
                    TEXT("Loaded object '%s' is not compatible with property class '%s'."),
                    *ObjectPath,
                    *ObjectProperty->PropertyClass->GetPathName());
                return false;
            }

            ObjectProperty->SetObjectPropertyValue(ValuePtr, LoadedObject);
            return true;
        }

        if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            FString EnumText;
            if (!JsonValue->TryGetString(EnumText))
            {
                OutError = TEXT("Expected an enum name string.");
                return false;
            }

            const UEnum* Enum = EnumProperty->GetEnum();
            const int64 EnumValue = Enum ? Enum->GetValueByNameString(EnumText) : INDEX_NONE;
            if (Enum == nullptr || EnumValue == INDEX_NONE)
            {
                OutError = FString::Printf(TEXT("Unknown enum value '%s'."), *EnumText);
                return false;
            }

            EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, EnumValue);
            return true;
        }

        if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
        {
            if (const UEnum* Enum = ByteProperty->Enum)
            {
                FString EnumText;
                if (!JsonValue->TryGetString(EnumText))
                {
                    OutError = TEXT("Expected an enum name string.");
                    return false;
                }

                const int64 EnumValue = Enum->GetValueByNameString(EnumText);
                if (EnumValue == INDEX_NONE)
                {
                    OutError = FString::Printf(TEXT("Unknown enum value '%s'."), *EnumText);
                    return false;
                }

                ByteProperty->SetPropertyValue(ValuePtr, static_cast<uint8>(EnumValue));
                return true;
            }
        }

        if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
        {
            if (StructProperty->Struct == TBaseStructure<FVector>::Get() || StructProperty->Struct == TBaseStructure<FRotator>::Get())
            {
                const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
                if (!JsonValue->TryGetArray(JsonArray) || JsonArray == nullptr || JsonArray->Num() != 3)
                {
                    OutError = TEXT("Expected a [X, Y, Z] array.");
                    return false;
                }

                double X = 0.0;
                double Y = 0.0;
                double Z = 0.0;
                if (!(*JsonArray)[0]->TryGetNumber(X) || !(*JsonArray)[1]->TryGetNumber(Y) || !(*JsonArray)[2]->TryGetNumber(Z))
                {
                    OutError = TEXT("Expected numeric vector/rotator components.");
                    return false;
                }

                if (StructProperty->Struct == TBaseStructure<FVector>::Get())
                {
                    *StructProperty->ContainerPtrToValuePtr<FVector>(Container) = FVector(X, Y, Z);
                }
                else
                {
                    *StructProperty->ContainerPtrToValuePtr<FRotator>(Container) = FRotator(X, Y, Z);
                }

                return true;
            }
        }

        OutError = FString::Printf(TEXT("Unsupported property type '%s'."), *Property->GetClass()->GetName());
        return false;
    }
}
