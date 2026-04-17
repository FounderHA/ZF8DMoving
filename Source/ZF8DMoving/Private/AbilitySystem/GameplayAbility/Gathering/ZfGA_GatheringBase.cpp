// Copyright ZfGame Studio. All Rights Reserved.
// ZfGA_GatherBase.cpp

#include "AbilitySystem/GameplayAbility/Gathering/ZfGA_GatheringBase.h"
#include "GatheringSystem/ZfGatheringComponent.h"
#include "GatheringSystem/ZfGatheringResourceData.h"
#include "Inventory/Fragments/ZfFragment_GatheringTool.h"
#include "Items/ZfItemPickup.h"
#include "Inventory/ZfEquipmentComponent.h"
#include "Inventory/ZfItemInstance.h"
#include "Inventory/ZfItemDefinition.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Tags/ZfGameplayTags.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"

// ============================================================
// Constructor
// ============================================================

UZfGA_GatheringBase::UZfGA_GatheringBase()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// ============================================================
// ActivateAbility
// ============================================================

void UZfGA_GatheringBase::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!Internal_ValidateAndSetup(TriggerEventData))
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    // Registra o input de hit diretamente — sem passar pelo sistema de interação
    Internal_BindHitInput();

    Internal_ExecuteNextHit();
}

// ============================================================
// CancelAbility
// ============================================================

void UZfGA_GatheringBase::CancelAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateCancelAbility)
{
    K2_OnGatherCancelled();
    Internal_Cleanup();
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

// ============================================================
// Internal_BindHitInput
// Registra o binding do botão de hit diretamente no PlayerController.
// Ativo apenas enquanto a ability estiver rodando.
// ============================================================

void UZfGA_GatheringBase::Internal_BindHitInput()
{
    if (!GatherHitInputAction)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGA_GatherBase: GatherHitInputAction não configurado na GA."));
        return;
    }

    APlayerController* PC = Cast<APlayerController>(
        GetActorInfo().PlayerController.Get());

    if (!PC) return;

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
    if (!EIC) return;

    HitInputHandle = EIC->BindAction(
        GatherHitInputAction,
        ETriggerEvent::Started,
        this,
        &UZfGA_GatheringBase::Internal_OnHitInputPressed).GetHandle();
}

// ============================================================
// Internal_UnbindHitInput
// Remove o binding — chamado no cleanup.
// ============================================================

void UZfGA_GatheringBase::Internal_UnbindHitInput()
{
    if (HitInputHandle == 0) return;

    APlayerController* PC = Cast<APlayerController>(
        GetActorInfo().PlayerController.Get());

    if (!PC) return;

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
    if (!EIC) return;

    EIC->RemoveBindingByHandle(HitInputHandle);
    HitInputHandle = 0;
}

// ============================================================
// Internal_OnHitInputPressed
// Chamado pelo Enhanced Input quando o jogador pressiona o botão.
// Repassa para o component — ele usa o CurrentAngle interno.
// ============================================================

void UZfGA_GatheringBase::Internal_OnHitInputPressed()
{
    if (TargetGatherableComponent)
    {
        TargetGatherableComponent->RegisterHit();
    }
}

// ============================================================
// Internal_ValidateAndSetup
// ============================================================

bool UZfGA_GatheringBase::Internal_ValidateAndSetup(const FGameplayEventData* TriggerEventData)
{
    if (!TriggerEventData || !TriggerEventData->OptionalObject)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGA_GatherBase: OptionalObject nulo."));
        return false;
    }

    const AActor* ResourceActor = Cast<AActor>(TriggerEventData->OptionalObject);
    if (!ResourceActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZfGA_GatherBase: OptionalObject não é um AActor."));
        return false;
    }

    TargetGatherableComponent = ResourceActor->FindComponentByClass<UZfGatheringComponent>();
    if (!TargetGatherableComponent)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGA_GatherBase: '%s' não tem ZfGatherableComponent."),
            *ResourceActor->GetName());
        return false;
    }

    if (!TargetGatherableComponent->IsAvailable())
    {
        UE_LOG(LogTemp, Warning, TEXT("ZfGA_GatherBase: Recurso esgotado."));
        return false;
    }

    TargetResourceData = TargetGatherableComponent->GetGatherResourceData();
    if (!TargetResourceData)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGA_GatherBase: GatherResourceData não configurado em '%s'."),
            *ResourceActor->GetName());
        return false;
    }

    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZfGA_GatherBase: AvatarActor nulo."));
        return false;
    }

    APlayerState* PS = AvatarActor->GetInstigatorController<APlayerController>()
        ? AvatarActor->GetInstigatorController<APlayerController>()->GetPlayerState<APlayerState>()
        : nullptr;

    UZfEquipmentComponent* EquipmentComponent = PS
        ? PS->FindComponentByClass<UZfEquipmentComponent>()
        : AvatarActor->FindComponentByClass<UZfEquipmentComponent>();

    if (!EquipmentComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZfGA_GatherBase: EquipmentComponent não encontrado."));
        return false;
    }

    UZfItemInstance* ToolInstance = EquipmentComponent->GetItemAtSlotTag(ToolSlotTag);
    if (!ToolInstance)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGA_GatherBase: Nenhum item equipado no slot '%s'."),
            *ToolSlotTag.ToString());
        return false;
    }

    const UZfFragment_GatheringTool* GatherFragment =
        ToolInstance->GetFragment<UZfFragment_GatheringTool>();
    if (!GatherFragment)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGA_GatherBase: Item no slot '%s' não tem ZfFragment_GatherTool."),
            *ToolSlotTag.ToString());
        return false;
    }

    ResolvedToolStats = GatherFragment->ResolveGatherStats(ToolInstance);
    if (!ResolvedToolStats.bIsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZfGA_GatherBase: ResolveGatherStats falhou."));
        return false;
    }

    if (!TargetResourceData->IsToolAllowed(ResolvedToolStats.ToolTag))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ZfGA_GatherBase: Ferramenta '%s' não aceita por '%s'."),
            *ResolvedToolStats.ToolTag.ToString(),
            *TargetResourceData->GetName());
        return false;
    }

    const FZfGatherToolEntry* ToolEntry =
        TargetResourceData->FindToolEntry(ResolvedToolStats.ToolTag);

    CachedResourceDamageMultiplier  = ToolEntry->DamageMultiplier;
    ResolvedToolStats.GoodSize      = ToolEntry->GoodSize;
    ResolvedToolStats.PerfectSize   = ToolEntry->PerfectSize;

    HitRecords.Empty();

    return true;
}

// ============================================================
// Internal_ExecuteNextHit
// ============================================================

void UZfGA_GatheringBase::Internal_ExecuteNextHit()
{
    if (TargetGatherableComponent->GetCurrentHP() <= 0.0f)
    {
        Internal_ResolveAndFinish();
        return;
    }

    K2_OnQTEStarted(
        ResolvedToolStats.GoodSize,
        ResolvedToolStats.PerfectSize,
        TargetResourceData->NeedleRotationTime);

    TargetGatherableComponent->BeginSkillCheckRound(
        ResolvedToolStats.GoodSize,
        ResolvedToolStats.PerfectSize,
        TargetResourceData->NeedleRotationTime,
        GetAvatarActorFromActorInfo());

    ActiveQTEWaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        ZfGatheringTags::QTE::Gathering_QTE_Hit,
        nullptr,
        false,
        false);

    ActiveQTEWaitTask->EventReceived.AddDynamic(
        this,
        &UZfGA_GatheringBase::Internal_OnQTEResultReceived);

    ActiveQTEWaitTask->ReadyForActivation();
}

// ============================================================
// Internal_OnQTEResultReceived
// ============================================================

void UZfGA_GatheringBase::Internal_OnQTEResultReceived(FGameplayEventData EventData)
{
    if (ActiveQTEWaitTask)
    {
        ActiveQTEWaitTask->EndTask();
        ActiveQTEWaitTask = nullptr;
    }

    const EZfGatherHitResult HitResult = Internal_TagToHitResult(EventData.EventTag);

    const float DamageMultiplierQTE = FZfGatherHitRecord::GetDamageMultiplierForResult(HitResult);
    const float DamageDealt =
        ResolvedToolStats.BaseDamage *
        CachedResourceDamageMultiplier *
        DamageMultiplierQTE;

    TargetGatherableComponent->ApplyDamage(DamageDealt);

    FZfGatherHitRecord Record;
    Record.HitResult   = HitResult;
    Record.DamageDealt = DamageDealt;
    Record.ScoreValue  = FZfGatherHitRecord::GetScoreForResult(HitResult);
    HitRecords.Add(Record);

    K2_OnHitImpact(HitResult, DamageDealt, TargetGatherableComponent->GetCurrentHP());

    UE_LOG(LogTemp, Warning,
    TEXT("GA: HitResult=%d | BaseDamage=%.1f | ResourceMult=%.2f | QTEMult=%.2f | DamageDealt=%.1f | HPRestante=%.1f"),
    (int32)HitResult,
    ResolvedToolStats.BaseDamage,
    CachedResourceDamageMultiplier,
    DamageMultiplierQTE,
    DamageDealt,
    TargetGatherableComponent->GetCurrentHP());
    
    Internal_ExecuteNextHit();
}

// ============================================================
// Internal_ResolveAndFinish
// ============================================================

void UZfGA_GatheringBase::Internal_ResolveAndFinish()
{
    const float FinalScore = Internal_CalculateFinalScore();
    const TArray<FZfGatherDropResult> Drops = Internal_ResolveLootTable(FinalScore);

    Internal_SpawnDrops(Drops);
    K2_OnDropsResolved(Drops, FinalScore);

    Internal_Cleanup();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// ============================================================
// Internal_CalculateFinalScore
// ============================================================

float UZfGA_GatheringBase::Internal_CalculateFinalScore() const
{
    if (HitRecords.IsEmpty()) return 0.0f;

    float ScoreSum = 0.0f;
    for (const FZfGatherHitRecord& Record : HitRecords)
    {
        ScoreSum += Record.ScoreValue;
    }

    const float RawScore = ScoreSum / static_cast<float>(HitRecords.Num());
    return FMath::Clamp(RawScore + ResolvedToolStats.ScoreBonus, 0.0f, 1.0f);
}

// ============================================================
// Internal_ResolveLootTable
// ============================================================

TArray<FZfGatherDropResult> UZfGA_GatheringBase::Internal_ResolveLootTable(float FinalScore) const
{
    TArray<FZfGatherDropResult> Drops;
    if (!TargetResourceData) return Drops;

    for (const FZfGatherLootEntry& Entry : TargetResourceData->LootTable)
    {
        if (FinalScore < Entry.ScoreMinimum) continue;
        if (FMath::FRand() > Entry.DropChance) continue;

        UZfItemDefinition* LoadedDefinition = Entry.ItemDefinition.LoadSynchronous();
        if (!LoadedDefinition)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ZfGA_GatherBase: Falha ao carregar ItemDefinition da loot table."));
            continue;
        }

        const int32 BaseQuantity  = FMath::RandRange(Entry.QuantityMin, Entry.QuantityMax);
        const int32 FinalQuantity = FMath::Max(
            1,
            FMath::RoundToInt(static_cast<float>(BaseQuantity) * ResolvedToolStats.DropMultiplier));

        FZfGatherDropResult Drop;
        Drop.ItemDefinition     = LoadedDefinition;
        Drop.Quantity           = FinalQuantity;
        Drop.ItemTier           = Entry.ItemTier;
        Drop.ItemRarity         = Entry.ItemRarity;
        Drop.SpawnScatterRadius = Entry.SpawnScatterRadius;
        Drops.Add(Drop);
    }

    return Drops;
}

// ============================================================
// Internal_SpawnDrops
// ============================================================

void UZfGA_GatheringBase::Internal_SpawnDrops(const TArray<FZfGatherDropResult>& Drops)
{
    if (GetWorld()->GetNetMode() == NM_Client) return;
    if (Drops.IsEmpty() || !TargetGatherableComponent) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const FVector ResourceLocation =
        TargetGatherableComponent->GetOwner()->GetActorLocation();

    for (const FZfGatherDropResult& Drop : Drops)
    {
        if (!Drop.ItemDefinition) continue;

        TSubclassOf<AZfItemPickup> PickupClass =
            Drop.ItemDefinition->ItemPickupActorClass.LoadSynchronous();

        if (!PickupClass)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ZfGA_GatherBase: '%s' não tem ItemPickupActorClass configurada."),
                *Drop.ItemDefinition->GetName());
            continue;
        }

        for (int32 i = 0; i < Drop.Quantity; i++)
        {
            const FVector RandomOffset = FVector(
                FMath::RandRange(-Drop.SpawnScatterRadius, Drop.SpawnScatterRadius),
                FMath::RandRange(-Drop.SpawnScatterRadius, Drop.SpawnScatterRadius),
                0.0f);

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            AZfItemPickup* Pickup = World->SpawnActor<AZfItemPickup>(
                PickupClass,
                ResourceLocation + RandomOffset,
                FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f),
                SpawnParams);

            if (!Pickup) continue;

            UZfItemInstance* NewInstance = NewObject<UZfItemInstance>(
                TargetGatherableComponent->GetOwner());

            NewInstance->InitializeItemInstance(
                Drop.ItemDefinition,
                Drop.ItemTier,
                Drop.ItemRarity);

            Pickup->InitializePickup(NewInstance);
        }
    }
}

// ============================================================
// Internal_TagToHitResult
// ============================================================

EZfGatherHitResult UZfGA_GatheringBase::Internal_TagToHitResult(
    const FGameplayTag& EventTag) const
{
    if (EventTag == ZfGatheringTags::QTE::Gathering_QTE_Hit_Perfect) return EZfGatherHitResult::Perfect;
    if (EventTag == ZfGatheringTags::QTE::Gathering_QTE_Hit_Good)    return EZfGatherHitResult::Good;
    if (EventTag == ZfGatheringTags::QTE::Gathering_QTE_Hit_Bad)     return EZfGatherHitResult::Bad;
    return EZfGatherHitResult::Missed;
}

// ============================================================
// Internal_Cleanup
// ============================================================

void UZfGA_GatheringBase::Internal_Cleanup()
{
    // Remove o binding de input antes de tudo
    Internal_UnbindHitInput();

    if (ActiveQTEWaitTask)
    {
        ActiveQTEWaitTask->EndTask();
        ActiveQTEWaitTask = nullptr;
    }

    if (TargetGatherableComponent)
    {
        TargetGatherableComponent->EndSkillCheck();
    }

    TargetGatherableComponent      = nullptr;
    TargetResourceData             = nullptr;
    ResolvedToolStats              = FZfResolvedGatherStats();
    CachedResourceDamageMultiplier = 1.0f;
    HitRecords.Empty();
}