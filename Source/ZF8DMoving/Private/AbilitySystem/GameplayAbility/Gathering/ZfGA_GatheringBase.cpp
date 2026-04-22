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
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "InteractionSystem/ZfInteractionComponent.h"

// ============================================================
// Constructor
// ============================================================

UZfGA_GatheringBase::UZfGA_GatheringBase()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

// ============================================================
// ActivateAbility
//
// Roda nos dois lados (servidor e cliente dono):
//   SERVIDOR: valida, commit, lock, cancellation listeners, inicia QTE
//   CLIENTE:  bind input (hit + move), bind delegate visual
// ============================================================

void UZfGA_GatheringBase::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // Validação e setup de referências — roda nos dois lados.
    // O cliente precisa de TargetGatherableComponent para bindar os delegates visuais.
    if (!Internal_ValidateAndSetup(TriggerEventData))
    {
        CancelAbility(Handle, ActorInfo, ActivationInfo, true);
        return;
    }

    // ── CLIENTE DONO — bind ANTES da lógica do servidor ─────────────
    // IMPORTANTE: No listen server HasAuthority e IsLocallyControlled são
    // ambos true. O bind precisa existir ANTES de Internal_ExecuteNextHit
    // chamar BeginSkillCheckRound que dispara OnSkillCheckRoundBegun.
    // Clientes remotos também passam por aqui (IsLocallyControlled = true
    // para o dono do ASC) e fazem o bind para receber o evento via RepNotify.
    if (ActorInfo && ActorInfo->IsLocallyControlled())
    {
        Internal_BindHitInput();
        Internal_BindClientDelegates();
    }

    // ── SERVIDOR ────────────────────────────────────────────────
    if (HasAuthority(&ActivationInfo))
    {
        if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
        {
            CancelAbility(Handle, ActorInfo, ActivationInfo, true);
            return;
        }

        // Trava o recurso — impede outros jogadores de coletar simultaneamente
        TargetGatherableComponent->StartGatheringLock(GetAvatarActorFromActorInfo());

        // Bind dos listeners de cancelamento (movimento, abilities, status tags)
        Internal_BindCancellationListeners();

        // Inicia o primeiro round do QTE — delegates do cliente já estão bindados
        Internal_ExecuteNextHit();
    }
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
// ============================================================

void UZfGA_GatheringBase::Client_NotifyGatherEnded_Implementation()
{
    K2_OnGatherCancelled();
}

void UZfGA_GatheringBase::Internal_BindHitInput()
{
    APlayerController* PC = Cast<APlayerController>(
        GetActorInfo().PlayerController.Get());
    if (!PC) return;

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
    if (!EIC) return;

    if (GatherHitInputAction)
    {
        HitInputHandle = EIC->BindAction(
            GatherHitInputAction,
            ETriggerEvent::Started,
            this,
            &UZfGA_GatheringBase::Internal_OnHitInputPressed).GetHandle();
    }
    else
    {
        UE_LOG(LogZfGathering, Warning,
            TEXT("ZfGA_GatherBase: GatherHitInputAction não configurado na GA."));
    }

    // Bind de movimento — cancela localmente para resposta imediata no cliente
    if (MoveInputAction)
    {
        MoveInputHandle = EIC->BindAction(
            MoveInputAction,
            ETriggerEvent::Started,
            this,
            &UZfGA_GatheringBase::Internal_OnMovementInputPressed).GetHandle();
    }
}

// ============================================================
// Internal_UnbindHitInput
// ============================================================

void UZfGA_GatheringBase::Internal_UnbindHitInput()
{
    APlayerController* PC = Cast<APlayerController>(
        GetActorInfo().PlayerController.Get());
    if (!PC) return;

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
    if (!EIC) return;

    if (HitInputHandle != 0)
    {
        EIC->RemoveBindingByHandle(HitInputHandle);
        HitInputHandle = 0;
    }

    if (MoveInputHandle != 0)
    {
        EIC->RemoveBindingByHandle(MoveInputHandle);
        MoveInputHandle = 0;
    }
}

// ============================================================
// Internal_OnHitInputPressed
// Cliente pressiona o botão → envia Server RPC.
// O servidor avalia o ângulo interno — cliente não envia dados de ângulo.
// ============================================================

void UZfGA_GatheringBase::Internal_OnHitInputPressed()
{
    Server_RegisterHit();
}

// ============================================================
// Internal_OnMovementInputPressed
// Cliente detecta movimento via input → cancela localmente (client prediction).
// O servidor também cancela via Internal_CheckMovement (timer).
// ============================================================

void UZfGA_GatheringBase::Internal_OnMovementInputPressed()
{
    CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

// ============================================================
// Server_RegisterHit_Implementation
// Executado no servidor quando o cliente pressiona o botão de hit.
// Chama RegisterHit() no componente que usa seu próprio ângulo interno.
// ============================================================

void UZfGA_GatheringBase::Server_RegisterHit_Implementation()
{
    if (TargetGatherableComponent)
    {
        TargetGatherableComponent->RegisterHit();
    }
}

// ============================================================
// Internal_BindCancellationListeners
// Chamado apenas no servidor.
// ============================================================

void UZfGA_GatheringBase::Internal_BindCancellationListeners()
{
    AActor* Avatar = GetAvatarActorFromActorInfo();
    if (!Avatar) return;

    // Salva posição inicial para detectar movimento
    GatherStartLocation = Avatar->GetActorLocation();

    // Timer de verificação de movimento (0.1s — funciona em dedicated server)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MovementCheckHandle,
            this,
            &UZfGA_GatheringBase::Internal_CheckMovement,
            0.1f,
            true); // looping
    }

    // Qualquer ability que ativar enquanto coleta roda cancela esta GA
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC)
    {
        AbilityActivatedDelegateHandle = ASC->AbilityActivatedCallbacks.AddUObject(
            this, &UZfGA_GatheringBase::Internal_OnAnyAbilityActivated);

        // Status tags que cancelam a coleta (hit react, knockback, etc.)
        for (const FGameplayTag& Tag : CancellationStatusTags)
        {
            FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(
                Tag,
                EGameplayTagEventType::NewOrRemoved).AddUObject(
                    this,
                    &UZfGA_GatheringBase::Internal_OnStatusTagChanged);

            StatusTagEventHandles.Add(Handle);
        }
    }
}

// ============================================================
// Internal_UnbindCancellationListeners
// ============================================================

void UZfGA_GatheringBase::Internal_UnbindCancellationListeners()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MovementCheckHandle);
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC)
    {
        if (AbilityActivatedDelegateHandle.IsValid())
        {
            ASC->AbilityActivatedCallbacks.Remove(AbilityActivatedDelegateHandle);
            AbilityActivatedDelegateHandle.Reset();
        }

        for (int32 i = 0; i < CancellationStatusTags.Num(); i++)
        {
            if (i < StatusTagEventHandles.Num() && StatusTagEventHandles[i].IsValid())
            {
                ASC->RegisterGameplayTagEvent(
                    CancellationStatusTags[i],
                    EGameplayTagEventType::NewOrRemoved).Remove(StatusTagEventHandles[i]);
            }
        }
        StatusTagEventHandles.Empty();
    }
}

// ============================================================
// Internal_CheckMovement
// Timer no servidor — verifica se o Avatar se moveu desde o início.
// Qualquer velocidade (> 1 cm/s) cancela a coleta.
// ============================================================

void UZfGA_GatheringBase::Internal_CheckMovement()
{
    AActor* Avatar = GetAvatarActorFromActorInfo();
    if (!Avatar)
    {
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
        return;
    }

    // Usa velocidade atual — mais confiável que delta de posição em altas taxas de tick
    if (ACharacter* Char = Cast<ACharacter>(Avatar))
    {
        if (Char->GetVelocity().SizeSquared() > 1.0f)
        {
            UE_LOG(LogZfGathering, Log, TEXT("ZfGA_GatherBase: Cancelado por movimento."));
            CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
        }
    }
}

// ============================================================
// Internal_OnAnyAbilityActivated
// Chamado quando qualquer ability ativa no ASC do jogador.
// Esta GA se cancela — o jogador está fazendo outra ação.
// ============================================================

void UZfGA_GatheringBase::Internal_OnAnyAbilityActivated(UGameplayAbility* ActivatedAbility)
{
    // Ignora se a ability que ativou for esta mesma (evita loop)
    if (ActivatedAbility == this) return;

    UE_LOG(LogZfGathering, Log,
        TEXT("ZfGA_GatherBase: Cancelado por ability '%s'."),
        *GetNameSafe(ActivatedAbility));

    CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

// ============================================================
// Internal_OnStatusTagChanged
// Chamado quando uma tag de status é adicionada ao ASC do jogador.
// Cancela a coleta se a tag foi ADICIONADA (NewCount > 0).
// ============================================================

void UZfGA_GatheringBase::Internal_OnStatusTagChanged(
    const FGameplayTag Tag, int32 NewCount)
{
    if (NewCount <= 0) return; // Tag removida — não cancela

    UE_LOG(LogZfGathering, Log,
        TEXT("ZfGA_GatherBase: Cancelado por status tag '%s'."),
        *Tag.ToString());

    CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

// ============================================================
// Internal_BindClientDelegates
// Bind no cliente para K2_OnClientRoundBegun (criar/exibir widget).
// ============================================================

void UZfGA_GatheringBase::Internal_BindClientDelegates()
{
    if (!TargetGatherableComponent) return;

    TargetGatherableComponent->OnSkillCheckRoundBegun.AddDynamic(
        this, &UZfGA_GatheringBase::Internal_OnClientRoundBegun);
}

// ============================================================
// Internal_UnbindClientDelegates
// ============================================================

void UZfGA_GatheringBase::Internal_UnbindClientDelegates()
{
    if (!TargetGatherableComponent) return;

    TargetGatherableComponent->OnSkillCheckRoundBegun.RemoveDynamic(
        this, &UZfGA_GatheringBase::Internal_OnClientRoundBegun);
}

// ============================================================
// Internal_OnClientRoundBegun
// Recebido no cliente quando um novo round chega via RepNotify.
// Chama K2_OnClientRoundBegun para que o Blueprint crie/mostre a widget.
// ============================================================

void UZfGA_GatheringBase::Internal_OnClientRoundBegun(
    float GoodStart, float GoodSize,
    float PerfectStart, float PerfectSize,
    float NeedleRotTime)
{
    // Cria a widget via Blueprint — só precisa acontecer UMA VEZ.
    // Rounds seguintes são gerenciados pela própria widget via Internal_OnRoundBegun.
    // Após esta chamada, a GA se desvincula para não criar outra widget a cada round.
    K2_OnClientRoundBegun(GoodSize, PerfectSize, NeedleRotTime);

    // Desvincula — a widget cuida dos rounds seguintes sozinha
    Internal_UnbindClientDelegates();
}

// ============================================================
// Internal_ValidateAndSetup
// ============================================================

bool UZfGA_GatheringBase::Internal_ValidateAndSetup(const FGameplayEventData* TriggerEventData)
{
    if (!TriggerEventData || !TriggerEventData->OptionalObject)
    {
        UE_LOG(LogZfGathering, Warning, TEXT("ZfGA_GatherBase: OptionalObject nulo."));
        return false;
    }

    const AActor* ResourceActor = Cast<AActor>(TriggerEventData->OptionalObject);
    if (!ResourceActor)
    {
        UE_LOG(LogZfGathering, Warning, TEXT("ZfGA_GatherBase: OptionalObject não é um AActor."));
        return false;
    }

    TargetGatherableComponent = ResourceActor->FindComponentByClass<UZfGatheringComponent>();
    if (!TargetGatherableComponent)
    {
        UE_LOG(LogZfGathering, Warning,
            TEXT("ZfGA_GatherBase: '%s' não tem ZfGatherableComponent."),
            *ResourceActor->GetName());
        return false;
    }

    // Estas verificações dependem de estado replicado (bIsDepleted, CurrentGatherer)
    // que pode estar stale no cliente. Só executa no servidor para evitar falsos
    // cancelamentos por latência que quebrariam o bind dos delegates visuais.
    if (TargetGatherableComponent->GetOwner()->HasAuthority())
    {
        if (TargetGatherableComponent->IsDepleted())
        {
            UE_LOG(LogZfGathering, Warning, TEXT("ZfGA_GatherBase: Recurso esgotado."));
            return false;
        }

        if (TargetGatherableComponent->IsBeingGathered())
        {
            UE_LOG(LogZfGathering, Warning, TEXT("ZfGA_GatherBase: Recurso já está sendo coletado."));
            return false;
        }
    }

    TargetResourceData = TargetGatherableComponent->GetGatherResourceData();
    if (!TargetResourceData)
    {
        UE_LOG(LogZfGathering, Warning,
            TEXT("ZfGA_GatherBase: GatherResourceData não configurado em '%s'."),
            *ResourceActor->GetName());
        return false;
    }

    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        UE_LOG(LogZfGathering, Warning, TEXT("ZfGA_GatherBase: AvatarActor nulo."));
        return false;
    }

    APlayerState* PS = nullptr;
    if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
    {
        PS = AvatarPawn->GetPlayerState<APlayerState>();
    }

    UZfEquipmentComponent* EquipmentComponent = PS
        ? PS->FindComponentByClass<UZfEquipmentComponent>()
        : AvatarActor->FindComponentByClass<UZfEquipmentComponent>();

    if (!EquipmentComponent)
    {
        UE_LOG(LogZfGathering, Warning, TEXT("ZfGA_GatherBase: EquipmentComponent não encontrado."));
        return false;
    }

    UZfItemInstance* ToolInstance = EquipmentComponent->GetItemAtSlotTag(ToolSlotTag);
    if (!ToolInstance)
    {
        UE_LOG(LogZfGathering, Warning,
            TEXT("ZfGA_GatherBase: Nenhum item no slot '%s'."),
            *ToolSlotTag.ToString());
        return false;
    }

    TArray<FZfAppliedModifier> AppliedModifiers = ToolInstance->GetAppliedModifiers();
    for (const FZfAppliedModifier& Modifier : AppliedModifiers)
    {
        if (Modifier.TargetType != EZfModifierTargetType::ItemProperty) continue;

        const FGameplayTag& Tag = Modifier.ItemPropertyTag;

        if (Tag == ZfItemPropertyTags::ToolProperties::Item_Gathering_ScoreBonus)
            CachedScoreBonus += Modifier.FinalValue;
        else if (Tag == ZfItemPropertyTags::ToolProperties::Item_Gathering_DamageBonus)
            CachedDamageBonus += Modifier.FinalValue;
        else if (Tag == ZfItemPropertyTags::ToolProperties::Item_Gathering_GoodSizeBonus)
            CachedGoodSizeBonus += Modifier.FinalValue / 100.f;
        else if (Tag == ZfItemPropertyTags::ToolProperties::Item_Gathering_PerfectSizeBonus)
            CachedPerfectSizeBonus += Modifier.FinalValue / 100.f;
        else if (Tag == ZfItemPropertyTags::ToolProperties::Item_Gathering_NeedleSpeedBonus)
            CachedNeedleTimeBonus += Modifier.FinalValue;
        
    }
        
    const UZfFragment_GatheringTool* GatherFragment = ToolInstance->GetFragment<UZfFragment_GatheringTool>();
    if (!GatherFragment)
    {
        UE_LOG(LogZfGathering, Warning,
            TEXT("ZfGA_GatherBase: Item no slot '%s' não tem ZfFragment_GatherTool."),
            *ToolSlotTag.ToString());
        return false;
    }

    ResolvedToolStats = GatherFragment->ResolveGatherStats(ToolInstance);
    if (!ResolvedToolStats.bIsValid)
    {
        UE_LOG(LogZfGathering, Warning, TEXT("ZfGA_GatherBase: ResolveGatherStats falhou."));
        return false;
    }

    if (!TargetResourceData->IsToolAllowed(ResolvedToolStats.ToolTag))
    {
        UE_LOG(LogZfGathering, Warning,
            TEXT("ZfGA_GatherBase: Ferramenta '%s' não aceita por '%s'."),
            *ResolvedToolStats.ToolTag.ToString(),
            *TargetResourceData->GetName());
        return false;
    }

    const FZfGatherToolEntry* ToolEntry =
        TargetResourceData->FindToolEntry(ResolvedToolStats.ToolTag);

    CachedResourceDamageMultiplier = ToolEntry->DamageMultiplier;
    ResolvedToolStats.GoodSize     = ToolEntry->GoodSize;
    ResolvedToolStats.PerfectSize  = ToolEntry->PerfectSize;

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
    FMath::Clamp(ResolvedToolStats.GoodSize * (1.0f + CachedGoodSizeBonus), 0.01f, 1.0f),
    FMath::Clamp(ResolvedToolStats.PerfectSize * (1.0f + CachedPerfectSizeBonus), 0.01f, 1.0f),
    FMath::Max(TargetResourceData->NeedleRotationTime + CachedNeedleTimeBonus, 0.1f));

    // Inicia o round no componente — gera zonas e replica ao cliente via RoundData
    TargetGatherableComponent->BeginSkillCheckRound(
    FMath::Clamp(ResolvedToolStats.GoodSize * (1.0f + CachedGoodSizeBonus), 0.01f, 1.0f),
    FMath::Clamp(ResolvedToolStats.PerfectSize * (1.0f + CachedPerfectSizeBonus), 0.01f, 1.0f),
    FMath::Max(TargetResourceData->NeedleRotationTime + CachedNeedleTimeBonus, 0.1f));

    // Aguarda o resultado do hit (GameplayEvent enviado por Internal_SendHitEvent)
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
    const float DamageDealt = (ResolvedToolStats.BaseDamage + CachedDamageBonus) * CachedResourceDamageMultiplier * DamageMultiplierQTE;

    TargetGatherableComponent->ApplyDamage(DamageDealt);

    FZfGatherHitRecord Record;
    Record.HitResult   = HitResult;
    Record.DamageDealt = DamageDealt;
    Record.ScoreValue  = FZfGatherHitRecord::GetScoreForResult(HitResult);
    HitRecords.Add(Record);

    K2_OnHitImpact(HitResult, DamageDealt, TargetGatherableComponent->GetCurrentHP());

    UE_LOG(LogZfGathering, Log,
        TEXT("ZfGA_GatherBase: Hit=%d | Dano=%.1f | HPRestante=%.1f"),
        (int32)HitResult, DamageDealt, TargetGatherableComponent->GetCurrentHP());

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
        ScoreSum += Record.ScoreValue;

    const float RawScore = ScoreSum / static_cast<float>(HitRecords.Num());
    return FMath::Clamp(RawScore + ResolvedToolStats.ScoreBonus + CachedScoreBonus, 0.0f, 1.0f);
}

// ============================================================
// Internal_ResolveLootTable
// ============================================================

TArray<FZfGatherDropResult> UZfGA_GatheringBase::Internal_ResolveLootTable(
    float FinalScore) const
{
    TArray<FZfGatherDropResult> Drops;
    if (!TargetResourceData) return Drops;

    for (const FZfGatherLootEntry& Entry : TargetResourceData->LootTable)
    {
        if (FinalScore < Entry.ScoreMinimum) continue;
        if (FMath::FRand() > Entry.DropChance) continue;

        UZfItemDefinition* LoadedDefinition = Entry.ItemDefinition.LoadSynchronous();
        if (!LoadedDefinition) continue;

        const int32 BaseQuantity = FMath::RandRange(Entry.QuantityMin, Entry.QuantityMax);
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
        if (!PickupClass) continue;

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
    // Notifica o cliente para fechar as widgets — antes de limpar qualquer referência
    if (HasAuthority(&CurrentActivationInfo))
    {
        Client_NotifyGatherEnded();
    }
    
    // Remove bindings de input (cliente)
    Internal_UnbindHitInput();
    Internal_UnbindClientDelegates();

    // Remove listeners de cancelamento (servidor)
    Internal_UnbindCancellationListeners();

    if (ActiveQTEWaitTask)
    {
        ActiveQTEWaitTask->EndTask();
        ActiveQTEWaitTask = nullptr;
    }

    // Para o QTE e libera o lock no componente
    if (TargetGatherableComponent)
    {
        TargetGatherableComponent->EndSkillCheck();
    }

    CachedScoreBonus        = 0.0f;
    CachedDamageBonus       = 0.0f;
    CachedGoodSizeBonus     = 0.0f;
    CachedPerfectSizeBonus  = 0.0f;
    CachedNeedleTimeBonus   = 0.0f;

    TargetGatherableComponent      = nullptr;
    TargetResourceData             = nullptr;
    ResolvedToolStats              = FZfResolvedGatherStats();
    CachedResourceDamageMultiplier = 1.0f;
    HitRecords.Empty();
    GatherStartLocation            = FVector::ZeroVector;
}