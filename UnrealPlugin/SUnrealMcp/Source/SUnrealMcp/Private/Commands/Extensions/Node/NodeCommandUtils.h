#pragma once

#include "Commands/Extensions/Blueprint/BlueprintCommandUtils.h"
#include "Commands/Extensions/Editor/EditorCommandUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "InputAction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnhancedInputAction.h"
#include "K2Node_Event.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/UObjectIterator.h"

namespace SUnrealMcpNodeCommandUtils
{
    inline UEdGraph* EnsureEventGraph(UBlueprint* Blueprint)
    {
        if (Blueprint == nullptr)
        {
            return nullptr;
        }

        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph != nullptr)
            {
                return Graph;
            }
        }

        UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
            Blueprint,
            TEXT("EventGraph"),
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());

        if (NewGraph != nullptr)
        {
            FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
        }

        return NewGraph;
    }

    inline bool TryReadVector2(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, FVector2D& OutVector)
    {
        const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, JsonArray) || JsonArray->Num() != 2)
        {
            return false;
        }

        double X = 0.0;
        double Y = 0.0;
        if (!(*JsonArray)[0]->TryGetNumber(X) || !(*JsonArray)[1]->TryGetNumber(Y))
        {
            return false;
        }

        OutVector = FVector2D(X, Y);
        return true;
    }

    inline UEdGraphNode* FindNodeByGuid(UEdGraph* Graph, const FString& NodeId)
    {
        if (Graph == nullptr || NodeId.IsEmpty())
        {
            return nullptr;
        }

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node != nullptr && Node->NodeGuid.ToString().Equals(NodeId, ESearchCase::IgnoreCase))
            {
                return Node;
            }
        }

        return nullptr;
    }

    inline UEdGraphPin* FindPinByName(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
    {
        if (Node == nullptr || PinName.IsEmpty())
        {
            return nullptr;
        }

        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin != nullptr && Pin->Direction == Direction && Pin->PinName.ToString() == PinName)
            {
                return Pin;
            }
        }

        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin != nullptr && Pin->Direction == Direction && Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
            {
                return Pin;
            }
        }

        return nullptr;
    }

    inline UClass* ResolveTargetClass(UBlueprint* Blueprint, const FString& Target)
    {
        if (Blueprint == nullptr)
        {
            return nullptr;
        }

        if (Target.IsEmpty() || Target.Equals(TEXT("self"), ESearchCase::IgnoreCase))
        {
            return Blueprint->SkeletonGeneratedClass != nullptr ? Blueprint->SkeletonGeneratedClass : Blueprint->GeneratedClass;
        }

        if (USCS_Node* ComponentNode = SUnrealMcpBlueprintCommandUtils::FindBlueprintComponentNode(Blueprint, Target))
        {
            return ComponentNode->ComponentTemplate != nullptr ? ComponentNode->ComponentTemplate->GetClass() : nullptr;
        }

        if (UObject* LoadedObject = LoadObject<UObject>(nullptr, *Target))
        {
            if (UClass* LoadedClass = Cast<UClass>(LoadedObject))
            {
                return LoadedClass;
            }

            return LoadedObject->GetClass();
        }

        UClass* ParentClass = SUnrealMcpBlueprintCommandUtils::ResolveParentClassReference(Target);
        if (ParentClass != nullptr)
        {
            return ParentClass;
        }

        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;
            if (Class != nullptr
                && (Class->GetName().Equals(Target, ESearchCase::IgnoreCase)
                    || Class->GetPathName().Equals(Target, ESearchCase::IgnoreCase)))
            {
                return Class;
            }
        }

        return nullptr;
    }

    inline UFunction* ResolveFunction(UBlueprint* Blueprint, const FString& Target, const FString& FunctionName)
    {
        if (Blueprint == nullptr || FunctionName.IsEmpty())
        {
            return nullptr;
        }

        UClass* TargetClass = ResolveTargetClass(Blueprint, Target);
        if (TargetClass == nullptr)
        {
            return nullptr;
        }

        for (UClass* Class = TargetClass; Class != nullptr; Class = Class->GetSuperClass())
        {
            if (UFunction* Function = Class->FindFunctionByName(*FunctionName))
            {
                return Function;
            }

            for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::IncludeSuper); It; ++It)
            {
                UFunction* Function = *It;
                if (Function != nullptr && Function->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
                {
                    return Function;
                }
            }
        }

        return nullptr;
    }

    inline bool ApplyJsonValueToPinDefault(const TSharedPtr<FJsonValue>& JsonValue, UEdGraphPin* Pin, FString& OutError)
    {
        if (!JsonValue.IsValid() || Pin == nullptr)
        {
            OutError = TEXT("Pin or parameter value is invalid.");
            return false;
        }

        const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
        const FName PinCategory = Pin->PinType.PinCategory;

        if (PinCategory == UEdGraphSchema_K2::PC_Boolean)
        {
            bool BoolValue = false;
            if (!JsonValue->TryGetBool(BoolValue))
            {
                OutError = TEXT("Expected a boolean value.");
                return false;
            }

            Schema->TrySetDefaultValue(*Pin, BoolValue ? TEXT("true") : TEXT("false"));
            return true;
        }

        if (PinCategory == UEdGraphSchema_K2::PC_Int || PinCategory == UEdGraphSchema_K2::PC_Byte || PinCategory == UEdGraphSchema_K2::PC_Int64)
        {
            double NumberValue = 0.0;
            if (!JsonValue->TryGetNumber(NumberValue))
            {
                OutError = TEXT("Expected a numeric value.");
                return false;
            }

            Schema->TrySetDefaultValue(*Pin, LexToString(static_cast<int64>(NumberValue)));
            return true;
        }

        if (PinCategory == UEdGraphSchema_K2::PC_Float || PinCategory == UEdGraphSchema_K2::PC_Real)
        {
            double NumberValue = 0.0;
            if (!JsonValue->TryGetNumber(NumberValue))
            {
                OutError = TEXT("Expected a numeric value.");
                return false;
            }

            Schema->TrySetDefaultValue(*Pin, FString::SanitizeFloat(NumberValue));
            return true;
        }

        if (PinCategory == UEdGraphSchema_K2::PC_String
            || PinCategory == UEdGraphSchema_K2::PC_Name
            || PinCategory == UEdGraphSchema_K2::PC_Text
            || PinCategory == UEdGraphSchema_K2::PC_Enum)
        {
            FString StringValue;
            if (!JsonValue->TryGetString(StringValue))
            {
                OutError = TEXT("Expected a string value.");
                return false;
            }

            Schema->TrySetDefaultValue(*Pin, StringValue);
            return true;
        }

        if (PinCategory == UEdGraphSchema_K2::PC_Struct)
        {
            const TArray<TSharedPtr<FJsonValue>>* ArrayValue = nullptr;
            if (!JsonValue->TryGetArray(ArrayValue) || ArrayValue == nullptr || ArrayValue->Num() != 3)
            {
                OutError = TEXT("Expected a [X, Y, Z] array.");
                return false;
            }

            double X = 0.0;
            double Y = 0.0;
            double Z = 0.0;
            if (!(*ArrayValue)[0]->TryGetNumber(X) || !(*ArrayValue)[1]->TryGetNumber(Y) || !(*ArrayValue)[2]->TryGetNumber(Z))
            {
                OutError = TEXT("Expected numeric struct values.");
                return false;
            }

            if (Pin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
            {
                Schema->TrySetDefaultValue(*Pin, FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z));
                return true;
            }

            if (Pin->PinType.PinSubCategoryObject == TBaseStructure<FRotator>::Get())
            {
                Schema->TrySetDefaultValue(*Pin, FString::Printf(TEXT("(Pitch=%f,Yaw=%f,Roll=%f)"), X, Y, Z));
                return true;
            }
        }

        if (PinCategory == UEdGraphSchema_K2::PC_Object || PinCategory == UEdGraphSchema_K2::PC_Class)
        {
            FString ObjectPath;
            if (!JsonValue->TryGetString(ObjectPath))
            {
                OutError = TEXT("Expected a string object path.");
                return false;
            }

            UObject* LoadedObject = LoadObject<UObject>(nullptr, *ObjectPath);
            if (LoadedObject == nullptr)
            {
                OutError = FString::Printf(TEXT("Could not load object '%s'."), *ObjectPath);
                return false;
            }

            Schema->TrySetDefaultObject(*Pin, LoadedObject);
            return true;
        }

        OutError = FString::Printf(TEXT("Unsupported pin category '%s'."), *PinCategory.ToString());
        return false;
    }

    inline TSharedPtr<FJsonObject> BuildNodeSummary(UEdGraphNode* Node)
    {
        const TSharedRef<FJsonObject> NodeObject = MakeShared<FJsonObject>();
        NodeObject->SetStringField(TEXT("id"), Node->NodeGuid.ToString());
        NodeObject->SetStringField(TEXT("class"), Node->GetClass()->GetPathName());
        NodeObject->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
        NodeObject->SetNumberField(TEXT("x"), Node->NodePosX);
        NodeObject->SetNumberField(TEXT("y"), Node->NodePosY);

        if (const UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
        {
            NodeObject->SetStringField(TEXT("eventName"), EventNode->EventReference.GetMemberName().ToString());
        }
        else if (const UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node))
        {
            NodeObject->SetStringField(TEXT("functionName"), FunctionNode->GetFunctionName().ToString());
        }
        else if (const UK2Node_EnhancedInputAction* InputNode = Cast<UK2Node_EnhancedInputAction>(Node))
        {
            NodeObject->SetStringField(TEXT("inputAction"), InputNode->InputAction != nullptr ? InputNode->InputAction->GetPathName() : TEXT(""));
        }

        return NodeObject;
    }
}
