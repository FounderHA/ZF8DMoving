// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibrary/ZfHelperLibrary.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Inventory/ZfInventoryComponent.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "Player/ZfPlayerState.h"


TArray<FGameplayAttribute> UZfHelperLibrary::GetAllAttributes(TSubclassOf<UAttributeSet> AttributeSetClass)
{
    TArray<FGameplayAttribute> Attributes;

    if (!AttributeSetClass)
        return Attributes;

    for (TFieldIterator<FProperty> It(AttributeSetClass); It; ++It)
    {
        FProperty* Property = *It;

        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            if (StructProp->Struct == TBaseStructure<FGameplayAttributeData>::Get())
            {
                FGameplayAttribute Attribute(Property);
                Attributes.Add(Attribute);
            }
        }
    }

    return Attributes;
}

UZfInventoryComponent* UZfHelperLibrary::FindInventoryComponent(AActor* Actor)
{
    if (!Actor) return nullptr;

    return Actor->FindComponentByClass<UZfInventoryComponent>();
}

UZfEquipmentComponent* UZfHelperLibrary::FindEquipmentComponent(AActor* Actor)
{
    if (!Actor) return nullptr;

    return Actor->FindComponentByClass<UZfEquipmentComponent>();
}

FString UZfHelperLibrary::ToRomanNumeral(int32 Number)
{
    if (Number <= 0 || Number > 3999)
        return FString::FromInt(Number);

    static const TArray<TPair<int32, FString>> RomanValues =
    {
        { 1000, TEXT("M")  },
        {  900, TEXT("CM") },
        {  500, TEXT("D")  },
        {  400, TEXT("CD") },
        {  100, TEXT("C")  },
        {   90, TEXT("XC") },
        {   50, TEXT("L")  },
        {   40, TEXT("XL") },
        {   10, TEXT("X")  },
        {    9, TEXT("IX") },
        {    5, TEXT("V")  },
        {    4, TEXT("IV") },
        {    1, TEXT("I")  },
    };

    FString Result;
    for (const TPair<int32, FString>& Pair : RomanValues)
    {
        while (Number >= Pair.Key)
        {
            Result += Pair.Value;
            Number -= Pair.Key;
        }
    }

    return Result;
}