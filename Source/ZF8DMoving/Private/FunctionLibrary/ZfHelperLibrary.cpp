// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibrary/ZfHelperLibrary.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Inventory/ZfInventoryComponent.h"
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
