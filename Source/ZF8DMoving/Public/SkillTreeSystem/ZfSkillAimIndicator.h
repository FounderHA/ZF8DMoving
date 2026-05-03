// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "ZfSkillAimIndicator.generated.h"

// =============================================================================
// EZfAimIndicatorType
// =============================================================================

UENUM(BlueprintType)
enum class EZfAimIndicatorType : uint8
{
	/** Círculo projetado no chão — skills de AoE com área de efeito. */
	GroundCircle    UMETA(DisplayName = "Ground Circle"),

	/**
	 * Reticle na direção da câmera — projéteis e skills direcionais.
	 * O indicador segue o ponto de impacto do trace de câmera no mundo.
	 */
	Reticle         UMETA(DisplayName = "Reticle"),
};

// =============================================================================
// AZfSkillAimIndicator
// =============================================================================

/**
 * Actor base do indicador visual de mira durante o AimMode de uma skill.
 *
 * Existe apenas no cliente local (owning client) — spawned e destroyed
 * pela UZfAbility_Active ao entrar e sair do AimMode.
 * Nunca replicado — puramente visual, sem impacto no servidor.
 *
 * Canal de colisão:
 *   AimTraceChannel = ECC_GameTraceChannel3 (SkillAim)
 *   Configurado em DefaultEngine.ini:
 *     +DefaultChannelResponses=(Channel=ECC_GameTraceChannel3,
 *       DefaultResponse=ECR_Block,bTraceType=True,bStaticObject=False,
 *       Name="SkillAim")
 *   Ajuste o número do canal se ECC_GameTraceChannel3 já estiver em uso.
 *
 * Comportamento quando não há hit (jogador olha para o céu):
 *   1. Trace câmera → mundo não encontra nada
 *   2. Projeta o ponto final horizontalmente no plano XY
 *   3. Faz um segundo trace vertical (downward) a partir desse ponto
 *   4. Se encontrar chão → indicador posicionado lá
 *   5. Se não encontrar → indicador posicionado no MaxRange sob o personagem
 *   O indicador nunca flutua no ar — sempre na superfície mais próxima.
 *
 * Parâmetros expostos ao Niagara:
 *   "User.HitLocation" → posição no mundo do ponto de mira
 *   "User.HitNormal"   → normal da superfície no ponto de impacto
 *   "User.Radius"      → raio do efeito (apenas GroundCircle)
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ZF8DMOVING_API AZfSkillAimIndicator : public AActor
{
	GENERATED_BODY()

public:

	AZfSkillAimIndicator();

	// ── Canal de colisão — fixo em C++ ───────────────────────────────────

	/**
	 * Canal de colisão do trace de mira.
	 * ECC_GameTraceChannel3 = "SkillAim" — configure no DefaultEngine.ini.
	 * Ajuste se necessário para o número de canal disponível no projeto.
	 */
	static constexpr ECollisionChannel AimTraceChannel = ECC_GameTraceChannel3;

	/**
	 * Canal usado pelo downward trace de fallback quando o trace
	 * primário não encontra superfície (jogador olhando para o céu).
	 * Mesmo canal do primário — busca o chão abaixo do ponto projetado.
	 */
	static constexpr ECollisionChannel AimFallbackChannel = ECC_GameTraceChannel3;

	// ── Parâmetros da skill — definidos por InitializeIndicator ──────────

	/**
	 * Alcance máximo da skill em unidades do mundo.
	 * 0 = sem limite (usa FallbackRange internamente).
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AimIndicator")
	float MaxRange = 1500.f;

	/** Raio do efeito — passado ao Niagara como "User.Radius". */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AimIndicator")
	float AimRadius = 300.f;

	/**
	 * Tipo de indicador — define o comportamento do trace.
	 * Configure no Blueprint filho com o tipo correto:
	 *   BP_AimIndicator_GroundCircle → GroundCircle
	 *   BP_AimIndicator_Reticle      → Reticle
	 * Lido por EnterAimMode na GA para passar ao InitializeIndicator.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AimIndicator")
	EZfAimIndicatorType IndicatorType = EZfAimIndicatorType::GroundCircle;

	// ── Componentes ───────────────────────────────────────────────────────

	/**
	 * Componente Niagara do indicador visual.
	 * Configure o NiagaraSystem no Blueprint filho.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AimIndicator")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	// ── Interface pública ─────────────────────────────────────────────────

	/**
	 * Inicializa o indicador com os parâmetros da skill.
	 * Chamado pela UZfAbility_Active imediatamente após o spawn.
	 *
	 * @param InMaxRange   Alcance máximo (GetEffectiveMaxRange da GA)
	 * @param InAimRadius  Raio do efeito (GetEffectiveAimRadius da GA)
	 * @param InType       Tipo de indicador
	 * @param InOwnerPawn  Pawn dono — origem do trace de câmera
	 */
	UFUNCTION(BlueprintCallable, Category = "AimIndicator")
	void InitializeIndicator(
		float InMaxRange,
		float InAimRadius,
		EZfAimIndicatorType InType,
		APawn* InOwnerPawn);

	/**
	 * Retorna a última posição válida calculada pelo trace.
	 * Consultado pela GA ao confirmar o cast para montar o TargetData.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AimIndicator")
	FVector GetCurrentHitLocation() const { return LastHitLocation; }

	/**
	 * Retorna a última normal de superfície válida calculada pelo trace.
	 * Consultado pela GA ao confirmar o cast para montar o TargetData.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AimIndicator")
	FVector GetCurrentHitNormal() const { return LastHitNormal; }

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Chamado após cada atualização do trace.
	 * Override no Blueprint filho para lógica visual adicional.
	 *
	 * @param HitLocation  Posição final do indicador no mundo
	 * @param HitNormal    Normal da superfície
	 * @param bHitValid    true se o trace primário encontrou superfície
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "AimIndicator")
	void OnIndicatorUpdated(
		const FVector& HitLocation,
		const FVector& HitNormal,
		bool bHitValid);
	virtual void OnIndicatorUpdated_Implementation(
		const FVector& HitLocation,
		const FVector& HitNormal,
		bool bHitValid) {}

private:

	/** Pawn dono — origem do trace de câmera. */
	UPROPERTY()
	TObjectPtr<APawn> OwnerPawn;

	/** Último ponto de mira válido. */
	FVector LastHitLocation = FVector::ZeroVector;

	/** Normal da superfície no último ponto válido. */
	FVector LastHitNormal = FVector::UpVector;

	/**
	 * Distância máxima do downward fallback trace.
	 * Quando o trace primário falha, o downward trace desce até esta
	 * distância abaixo do ponto projetado para encontrar o chão.
	 */
	static constexpr float DownwardTraceDistance = 10000.f;

	/**
	 * Executa o trace e atualiza a posição do indicador.
	 * Chamado a cada Tick.
	 *
	 * Fluxo:
	 *   1. Trace câmera → mundo (AimTraceChannel)
	 *   2. Hit → clamp ao MaxRange → posiciona indicador
	 *   3. Sem hit → projeta ponto no plano XY → downward trace
	 *   4. Downward hit → posiciona indicador no chão encontrado
	 *   5. Downward sem hit → posiciona no MaxRange sob o personagem
	 */
	void UpdateTrace();

	/**
	 * Trace vertical de fallback — usado quando o trace primário
	 * não encontra superfície (câmera apontada para o céu).
	 *
	 * @param ProjectedPoint  Ponto projetado horizontalmente no MaxRange
	 * @param OutHitLocation  Resultado: posição no chão encontrado
	 * @param OutHitNormal    Resultado: normal da superfície
	 * @return true se encontrou superfície abaixo do ponto
	 */
	bool DownwardTrace(
		const FVector& ProjectedPoint,
		FVector& OutHitLocation,
		FVector& OutHitNormal) const;

	/** Atualiza posição do Actor e parâmetros do NiagaraComponent. */
	void UpdateNiagaraParameters(
		const FVector& HitLocation,
		const FVector& HitNormal);

	/**
	 * Retorna origem e direção do trace a partir da câmera do OwnerPawn.
	 * @return false se PlayerController não encontrado
	 */
	bool GetCameraViewPoint(FVector& OutOrigin, FVector& OutDirection) const;
};