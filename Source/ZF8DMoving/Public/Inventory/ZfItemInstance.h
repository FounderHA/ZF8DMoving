// Copyright ZfGame Studio. All Rights Reserved.
// ZfItemInstance.h
// UObject que armazena todas as informações DINÂMICAS de um item.
//
// CONCEITO:
// O ItemInstance representa "um item específico" do jogador.
// Ex: "A espada de ferro do João com 73 de durabilidade,
//      quality 3, tier 2 e modifiers X e Y aplicados."
//
// SEPARAÇÃO DE RESPONSABILIDADES:
// - ItemDefinition (PDA)  → dados estáticos (nunca mudam)
// - ItemInstance (UObject) → dados dinâmicos (mudam durante gameplay)
//
// REPLICAÇÃO:
// O ItemInstance NÃO replica a si mesmo diretamente.
// Ele é replicado DENTRO do FFastArraySerializer (FZfInventoryList)
// pelo UZfInventoryComponent. Toda mudança de dado deve ser feita
// no servidor e será replicada automaticamente para os clientes.
//
// ACESSO AOS FRAGMENTS:
// O ItemInstance acessa os fragments via seu ItemDefinition:
// const UZfFragment_Durability* F = GetFragment<UZfFragment_Durability>();
//
// OUTER:
// O Outer do ItemInstance deve ser sempre o OwnerActor (o Character).
// Isso garante que o UObject existe no contexto correto da rede.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "ZfInventoryTypes.h"
#include "ZfModifierDataTypes.h"
#include "ZfItemDefinition.h"
#include "ZfItemInstance.generated.h"

// Forward declarations
class UZfItemFragment;
class UZfInventoryComponent;
class UZfEquipmentComponent;
class UAbilitySystemComponent;

// ============================================================
// UZfItemInstance
// ============================================================

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Within = Actor)
class ZF8DMOVING_API UZfItemInstance : public UObject
{
    GENERATED_BODY()

public:

    UZfItemInstance();

    // ----------------------------------------------------------
    // INICIALIZAÇÃO
    // Chamado pelo InventoryComponent ao criar uma nova instância.
    // Deve ser chamado APENAS no servidor.
    // ----------------------------------------------------------

    // Inicializa o ItemInstance com sua definição e tier.
    // Gera o GUID único, copia os dados base do ItemDefinition
    // e rola os modifiers iniciais se aplicável.
    // @param InItemDefinition — o PDA que define este item
    // @param InItemTier — tier do item (0 a 5)
    // @param InItemRarity — raridade do item
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    void InitializeItemInstance(UZfItemDefinition* InItemDefinition, int32 InItemTier, EZfItemRarity InItemRarity);

    // ----------------------------------------------------------
    // IDENTIFICAÇÃO
    // ----------------------------------------------------------

    // GUID único gerado ao criar o item — nunca muda.
    // Usado para identificar o item na rede e no save game.
    // Replicado para que clientes possam identificar o item.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Identity")
    FGuid ItemGuid;

    // Referência ao ItemDefinition (PDA) deste item.
    // Soft reference replicada — clientes carregam o asset localmente.
    // NUNCA modifique após a inicialização.
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Item|Identity")
    TObjectPtr<UZfItemDefinition> ItemDefinition;

    // ----------------------------------------------------------
    // PROGRESSÃO
    // ----------------------------------------------------------

    // Tier do item (0 a 5) — define o poder base e
    // quais ranks de modifier podem aparecer.
    // Definido ao dropar o item e nunca muda.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Progression")
    int32 ItemTier = 0;

    // Raridade do item — influencia quantidade de modifiers
    // e valor de mercado.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Progression")
    EZfItemRarity ItemRarity = EZfItemRarity::Common;

    // Qualidade atual do item (0 a MAX_ITEM_QUALITY = 9).
    // Aumenta via mecânica de upgrade usando o QualityDataTable.
    // Escala os stats base ao ser alterada.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Progression")
    int32 CurrentQuality = 0;

    // ----------------------------------------------------------
    // STACK
    // Apenas relevante para itens com UZfFragment_Stackable.
    // ----------------------------------------------------------

    // Quantidade atual de itens neste stack.
    // Ex: 15 poções de vida em um slot.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Stack")
    int32 CurrentStack = 1;

    // ----------------------------------------------------------
    // DURABILIDADE
    // Apenas relevante para itens com UZfFragment_Durability.
    // ----------------------------------------------------------

    // Durabilidade atual do item.
    // Quando chega a 0: bônus desativados, item permanece equipado,
    // Widget notifica o player.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Durability")
    float CurrentDurability = 0.0f;

    // Se verdadeiro, o item pode ser reparado.
    // Dinâmico — pode ser alterado via gameplay
    // (ex: corrupção pode bloquear o reparo).
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Durability")
    bool bIsRepairable = true;

    // Se verdadeiro, o item está quebrado (CurrentDurability == 0).
    // Todos os bônus são desativados enquanto este flag estiver ativo.
    // Replicado para que a UI do cliente possa reagir.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Durability")
    bool bIsBroken = false;

    // ----------------------------------------------------------
    // STATS BASE
    // Valores escalados por ItemTier + CurrentQuality.
    // Apenas os stats relevantes ao tipo de item terão
    // valores diferentes de 0.
    // Recalculado quando qualidade, modifiers ou set bonus mudam.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Attributes")
    TArray<FZfItemAttributeValue> ItemAttributes;
    
    // Recalcula todos os atributos do item baseado na qualidade atual e modifiers.
    // Deve ser chamado quando qualidade ou modifiers mudarem.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    void RecalculateItemAttributes();

    // ----------------------------------------------------------
    // MODIFIERS
    // Lista de modifiers rolados e aplicados neste item.
    // Gerenciados pelo sistema de modifier em runtime.
    // ----------------------------------------------------------

    // Modifiers ativos neste item.
    // Cada entrada é um modifier já rolado com rank e valor definidos.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Modifiers")
    TArray<FZfAppliedModifier> AppliedModifiers;

    // ----------------------------------------------------------
    // CORRUPÇÃO
    // ----------------------------------------------------------

    // Estado de corrupção atual do item.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Corruption")
    EZfCorruptionState CorruptionState = EZfCorruptionState::None;

    // ----------------------------------------------------------
    // MARKET VALUE
    // Calculado automaticamente em runtime baseado em:
    // BaseMarketValue (ItemDefinition) * RarityMultiplier *
    // TierMultiplier + ModifierRankBonus
    // ----------------------------------------------------------

    // Valor de venda calculado para NPCs.
    // Recalculado sempre que modifiers, tier ou quality mudam.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Economy")
    float CalculatedMarketValue = 0.0f;
    
    // ----------------------------------------------------------
    // FUNÇÕES DE ACESSO AOS FRAGMENTS
    // Acessa os fragments via ItemDefinition.
    // ----------------------------------------------------------

    // Busca um fragment do tipo T no ItemDefinition deste item.
    // Retorna nullptr se o fragment não existir ou ItemDefinition for nulo.
    // Uso: const UZfFragment_Durability* F = Instance->GetFragment<UZfFragment_Durability>();
    template<typename T>
    const T* GetFragment() const
    {
        static_assert(TIsDerivedFrom<T, UZfItemFragment>::IsDerived, "T deve ser uma subclasse de UZfItemFragment.");

        if (!ItemDefinition)
        {
            UE_LOG(LogZfInventory, Warning,
                TEXT("UZfItemInstance::GetFragment — ItemDefinition é nulo. " "GUID: %s"), *ItemGuid.ToString());
            return nullptr;
        }

        return ItemDefinition->FindFragment<T>();
    }

    // Verifica se este item possui um fragment do tipo T.
    template<typename T>
    bool HasFragment() const
    {
        return GetFragment<T>() != nullptr;
    }

    // Retorna um fragment do item pelo tipo passado.
    // @param FragmentClass — classe do fragment a buscar
    // @return fragment encontrado ou nullptr
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Fragments")
    UZfItemFragment* GetFragmentByClass(TSubclassOf<UZfItemFragment> FragmentClass) const;
    
    // ----------------------------------------------------------
    // FUNÇÕES DE ACESSO AOS DADOS
    // ----------------------------------------------------------

    // Retorna o GUID único deste item.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    FGuid GetItemGuid() const { return ItemGuid; }

    // Retorna o ItemDefinition deste item.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    UZfItemDefinition* GetItemDefinition() const { return ItemDefinition; }

    // Retorna o nome do item via ItemDefinition.
    // Retorna texto vazio se ItemDefinition for nulo.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    FText GetItemName() const;

    // Retorna as tags do item via ItemDefinition.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    FGameplayTagContainer GetItemTags() const;

    // Verifica se o item tem uma tag específica.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    bool HasItemTag(const FGameplayTag& Tag) const;

    // ----------------------------------------------------------
    // FUNÇÕES DE MODIFICAÇÃO — apenas servidor
    // Todas as funções abaixo devem ser chamadas apenas no servidor.
    // As mudanças são replicadas automaticamente via InventoryComponent.
    // ----------------------------------------------------------

    // Define a quantidade do stack.
    // @param NewStack — nova quantidade (clampada entre 1 e MaxStackSize)
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server",
        meta = (ToolTip = "Chamar apenas no servidor."))
    void SetCurrentStack(int32 NewStack);

    // ZfItemInstance.h
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    void SetQuality(int32 NewQuality);
    
    // Adiciona quantidade ao stack atual.
    // Retorna a quantidade que não coube (overflow).
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server")
    int32 AddToStack(int32 AmountToAdd);

    // Remove quantidade do stack atual.
    // Retorna true se o stack chegou a 0 (item deve ser removido).
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server")
    bool RemoveFromStack(int32 AmountToRemove);

    // Aplica dano de durabilidade ao item.
    // Se chegar a 0, chama OnItemBroken em todos os fragments
    // e notifica o EquipmentComponent.
    // @param DamageAmount — quantidade de durabilidade a remover
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server")
    void ApplyDurabilityDamage(float DamageAmount);

    // Repara o item para a durabilidade máxima definida no fragment.
    // Chama OnItemRepaired em todos os fragments se estava quebrado.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server")
    void RepairItem();

    // Repara o item para uma durabilidade específica.
    // @param RepairAmount — quantidade de durabilidade a restaurar
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server")
    void RepairItemByAmount(float RepairAmount);
    

    // Recalcula e atualiza o valor de mercado do item.
    // Chamado automaticamente ao mudar modifiers, tier ou quality.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server")
    void RecalculateMarketValue();

    // Define o estado de reparo do item.
    // Pode ser alterado via gameplay (ex: corrupção bloqueia reparo).
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Server")
    void SetIsRepairable(bool bRepairable);
    
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    void SetRarity(EZfItemRarity NewRarity);

    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    void SetTier(int32 NewTier);

    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    void SetItemDefinition(UZfItemDefinition* InItemDefinition);
    
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance")
    void SetAppliedModifiers(const TArray<FZfAppliedModifier>& NewModifiers);
    
    // ----------------------------------------------------------
    // FUNÇÕES DE MODIFIER
    // ----------------------------------------------------------

    // Adiciona um modifier já rolado a este item.
    // Valida limite de classe e duplicatas antes de adicionar.
    // Retorna false se não for possível adicionar.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Modifier")
    bool AddAppliedModifier(const FZfAppliedModifier& NewModifier);

    // Remove um modifier pelo nome da linha na DataTable.
    // Retorna false se o modifier não foi encontrado.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Modifier")
    bool RemoveAppliedModifier(const FName& ModifierRowName);

    // Remove todos os modifiers não-debuff (usado pelo Reroll All).
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Modifier")
    void RemoveAllNonDebuffModifiers();

    // Busca um modifier aplicado pelo nome da linha.
    // Retorna nullptr se não encontrado.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Modifier")
    const FZfAppliedModifier& FindAppliedModifier(const FName& ModifierRowName);

    // Verifica se este item já tem um modifier específico.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Modifier")
    bool HasAppliedModifier(const FName& ModifierRowName) const;

    // Verifica se o item pode receber mais modifiers da classe especificada.
    // Respeita os limites definidos em FZfItemModifierConfig.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Modifier")
    bool CanAddModifierOfClass(EZfModifierClass ModifierClass) const;

    // Conta quantos modifiers da classe especificada este item tem.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Modifier")
    int32 CountModifiersOfClass(EZfModifierClass ModifierClass) const;

    // ----------------------------------------------------------
    // CORRUPÇÃO
    // ----------------------------------------------------------

    // Corrompe o item — adiciona um modifier de debuff aleatório.
    // Retorna false se o item já está corrompido ou se não há
    // modifiers de debuff disponíveis no DataTable.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Corruption")
    bool CorruptItem();

    // Verifica se o item está corrompido.
    UFUNCTION(BlueprintCallable, Category = "Zf|ItemInstance|Corruption")
    bool IsCorrupted() const
    {
        return CorruptionState == EZfCorruptionState::Corrupted;
    }

    // ----------------------------------------------------------
    // NOTIFICAÇÃO AOS FRAGMENTS
    // Propaga eventos para todos os fragments do ItemDefinition.
    // ----------------------------------------------------------

    // Notifica todos os fragments que o item foi adicionado ao inventário.
    void NotifyFragments_ItemAddedToInventory(UZfInventoryComponent* InventoryComponent);

    // Notifica todos os fragments que o item foi removido do inventário.
    void NotifyFragments_ItemRemovedFromInventory(UZfInventoryComponent* InventoryComponent);

    // Notifica todos os fragments que o item foi equipado.
    void NotifyFragments_ItemEquipped(
        UZfEquipmentComponent* EquipmentComponent,
        AActor* EquippingActor);

    // Notifica todos os fragments que o item foi desequipado.
    void NotifyFragments_ItemUnequipped(
        UZfEquipmentComponent* EquipmentComponent,
        AActor* UnequippingActor);

    // Notifica todos os fragments que o item quebrou.
    void NotifyFragments_ItemBroken();

    // Notifica todos os fragments que o item foi reparado.
    void NotifyFragments_ItemRepaired();

    // ----------------------------------------------------------
    // REPLICAÇÃO
    // ----------------------------------------------------------

    // Registra as propriedades que serão replicadas pela rede.
    // Chamado automaticamente pelo Unreal antes de replicar.
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Necessário para que UObjects possam ser replicados
    // dentro do FFastArraySerializer do InventoryComponent.
    virtual bool IsSupportedForNetworking() const override { return true; }

private:

    // ----------------------------------------------------------
    // FUNÇÕES INTERNAS
    // ----------------------------------------------------------

    // Inicializa os modifiers para itens Unique (modifiers fixos do PDA).
    // Para itens normais, os modifiers são rolados pelo InventoryComponent.
    void Internal_InitializeUniqueModifiers();

    // Verifica se este código está rodando no servidor.
    // Loga um warning se uma operação de servidor for chamada no cliente.
    bool Internal_CheckIsServer(const FString& FunctionName) const;


public:
    
    // ----------------------------------------------------------
    // DEBUG
    // ----------------------------------------------------------

    
};