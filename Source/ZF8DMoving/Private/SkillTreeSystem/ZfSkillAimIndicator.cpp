// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillTreeSystem/ZfSkillAimIndicator.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// =============================================================================
// Constructor
// =============================================================================

AZfSkillAimIndicator::AZfSkillAimIndicator()
{
	// Tick desativado até InitializeIndicator ser chamado
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Sem replicação — existe apenas no cliente local
	bReplicates = false;
	bAlwaysRelevant = false;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	RootComponent = NiagaraComponent;
}

// =============================================================================
// BeginPlay
// =============================================================================

void AZfSkillAimIndicator::BeginPlay()
{
	Super::BeginPlay();
}

// =============================================================================
// InitializeIndicator
// =============================================================================

void AZfSkillAimIndicator::InitializeIndicator(
	float InMaxRange,
	float InAimRadius,
	EZfAimIndicatorType InType,
	APawn* InOwnerPawn)
{
	MaxRange      = InMaxRange;
	AimRadius     = InAimRadius;
	IndicatorType = InType;
	OwnerPawn     = InOwnerPawn;

	// Passa o raio inicial ao Niagara
	if (NiagaraComponent)
	{
		NiagaraComponent->SetVariableFloat(FName("User.Radius"), AimRadius);
	}

	// Ativa o tick agora que está inicializado
	SetActorTickEnabled(true);
}

// =============================================================================
// Tick
// =============================================================================

void AZfSkillAimIndicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTrace();
}

// =============================================================================
// UpdateTrace
// =============================================================================

void AZfSkillAimIndicator::UpdateTrace()
{
	if (!OwnerPawn) return;

	FVector CameraOrigin;
	FVector CameraDirection;
	if (!GetCameraViewPoint(CameraOrigin, CameraDirection)) return;

	// Distância do trace primário: MaxRange > 0 → usa MaxRange * 2 para
	// garantir que sempre atinge uma superfície dentro do alcance.
	// MaxRange == 0 → sem limite, usa distância grande mas finita.
	const float TraceDistance = (MaxRange > 0.f) ? MaxRange * 2.f : 50000.f;
	const FVector TraceEnd    = CameraOrigin + CameraDirection * TraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerPawn);

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraOrigin,
		TraceEnd,
		AimTraceChannel,
		QueryParams);

	FVector FinalLocation;
	FVector FinalNormal;

	if (bHit)
	{
		// ── Trace primário encontrou superfície ───────────────────────────
		FinalLocation = HitResult.ImpactPoint;
		FinalNormal   = HitResult.ImpactNormal;

		// Clamp ao MaxRange a partir do personagem
		if (MaxRange > 0.f)
		{
			const FVector PawnLocation = OwnerPawn->GetActorLocation();
			const FVector ToHit        = FinalLocation - PawnLocation;

			if (ToHit.SizeSquared() > FMath::Square(MaxRange))
			{
				// Ponto além do alcance — projeta no MaxRange na mesma direção
				FinalLocation = PawnLocation + ToHit.GetSafeNormal() * MaxRange;

				// Downward trace a partir do ponto clampado para garantir
				// que o indicador fique na superfície correta
				FVector DownLocation, DownNormal;
				if (DownwardTrace(FinalLocation, DownLocation, DownNormal))
				{
					FinalLocation = DownLocation;
					FinalNormal   = DownNormal;
				}
			}
		}
	}
	else
	{
		// ── Trace primário sem hit — câmera apontada para o céu ou vazio ──
		//
		// Estratégia:
		//   1. Projeta o ponto final da câmera horizontalmente no MaxRange
		//      a partir do personagem (ignora altura da câmera)
		//   2. Faz downward trace a partir desse ponto para encontrar o chão
		//   3. Se não encontrar chão, usa MaxRange sob o personagem

		const FVector PawnLocation = OwnerPawn->GetActorLocation();
		const float   ClampedRange = (MaxRange > 0.f) ? MaxRange : 1500.f;

		// Direção horizontal — ignora componente Z da câmera
		FVector HorizontalDir = CameraDirection;
		HorizontalDir.Z       = 0.f;

		if (!HorizontalDir.IsNearlyZero())
		{
			HorizontalDir.Normalize();
		}
		else
		{
			// Câmera apontada diretamente para cima ou baixo —
			// usa forward do personagem como fallback
			HorizontalDir = OwnerPawn->GetActorForwardVector();
			HorizontalDir.Z = 0.f;
			HorizontalDir.Normalize();
		}

		// Ponto projetado horizontalmente no MaxRange
		const FVector ProjectedPoint = PawnLocation + HorizontalDir * ClampedRange;

		// Downward trace a partir do ponto projetado
		FVector DownLocation, DownNormal;
		if (DownwardTrace(ProjectedPoint, DownLocation, DownNormal))
		{
			FinalLocation = DownLocation;
			FinalNormal   = DownNormal;
		}
		else
		{
			// Fallback final — sob o personagem no MaxRange
			FinalLocation = PawnLocation + HorizontalDir * ClampedRange;
			FinalLocation.Z = PawnLocation.Z;
			FinalNormal   = FVector::UpVector;
		}
	}

	LastHitLocation = FinalLocation;
	LastHitNormal   = FinalNormal;

	SetActorLocation(FinalLocation);
	UpdateNiagaraParameters(FinalLocation, FinalNormal);
	OnIndicatorUpdated(FinalLocation, FinalNormal, bHit);
}

// =============================================================================
// DownwardTrace
// =============================================================================

bool AZfSkillAimIndicator::DownwardTrace(
	const FVector& ProjectedPoint,
	FVector& OutHitLocation,
	FVector& OutHitNormal) const
{
	// Começa bem acima do ponto projetado para garantir que não
	// começa dentro de uma superfície
	const FVector TraceStart = ProjectedPoint + FVector(0.f, 0.f, 500.f);
	const FVector TraceEnd   = ProjectedPoint - FVector(0.f, 0.f, DownwardTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(const_cast<AZfSkillAimIndicator*>(this));
	if (OwnerPawn)
	{
		QueryParams.AddIgnoredActor(OwnerPawn);
	}

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		AimFallbackChannel,
		QueryParams);

	if (bHit)
	{
		OutHitLocation = HitResult.ImpactPoint;
		OutHitNormal   = HitResult.ImpactNormal;
	}

	return bHit;
}

// =============================================================================
// UpdateNiagaraParameters
// =============================================================================

void AZfSkillAimIndicator::UpdateNiagaraParameters(
	const FVector& HitLocation,
	const FVector& HitNormal)
{
	if (!NiagaraComponent) return;

	NiagaraComponent->SetVariableVec3(FName("User.HitLocation"), HitLocation);
	NiagaraComponent->SetVariableVec3(FName("User.HitNormal"),   HitNormal);
	// User.Radius é setado apenas em InitializeIndicator —
	// não muda por tick a menos que a GA atualize via SetAimRadius
}

// =============================================================================
// GetCameraViewPoint
// =============================================================================

bool AZfSkillAimIndicator::GetCameraViewPoint(
	FVector& OutOrigin,
	FVector& OutDirection) const
{
	if (!OwnerPawn) return false;

	const APlayerController* PC =
		Cast<APlayerController>(OwnerPawn->GetController());

	if (!PC) return false;

	FRotator CameraRotation;
	PC->GetPlayerViewPoint(OutOrigin, CameraRotation);
	OutDirection = CameraRotation.Vector();

	return true;
}