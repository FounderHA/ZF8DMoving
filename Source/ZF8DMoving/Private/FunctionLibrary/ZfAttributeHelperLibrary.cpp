// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibrary/ZfAttributeHelperLibrary.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

TArray<FGameplayAttribute> UZfAttributeHelperLibrary::GetAllAttributes(TSubclassOf<UAttributeSet> AttributeSetClass)
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
