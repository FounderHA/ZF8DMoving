#include "GatheringSystem/ZfResourceHPWidget.h"
#include "GatheringSystem/ZfGatheringComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GatheringSystem/ZfGatheringResourceData.h"

void UZfResourceHPWidget::InitResourceWidget(AActor* InResourceActor)
{
	if (!InResourceActor) return;

	ResourceActor = InResourceActor;

	if (UZfGatheringComponent* GC = ResourceActor->FindComponentByClass<UZfGatheringComponent>())
	{
		GC->OnResourceHPChanged.AddDynamic(this, &UZfResourceHPWidget::BP_OnHPChanged);
		BP_OnHPChanged(GC->GetCurrentHP(), GC->GetMaxHP());

		// Lê o offset do data asset do recurso
		if (UZfGatheringResourceData* Data = GC->GetGatherResourceData())
		{
			WorldOffset = Data->HPWidgetOffset;
		}
	}

	UpdateScreenPosition();
}

void UZfResourceHPWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateScreenPosition();
}

void UZfResourceHPWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetAlignmentInViewport(FVector2D(0.5f, .5f));
}

void UZfResourceHPWidget::UpdateScreenPosition()
{
	if (!ResourceActor) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	const FVector WorldLocation = ResourceActor->GetActorLocation() + WorldOffset;

	FVector2D ScreenPosition;
	const bool bIsOnScreen = PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPosition, false);
	
	//FVector2D IndicatorOffset = ScreenPosition; //IZfInteractionInterface::Execute_GetIndicatorWidgetOffset(Actor);

	//Widget->SetVisibility(bOnScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	
	if (bIsOnScreen != bWasOnScreen)
	{
		SetVisibility(bIsOnScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (bIsOnScreen)
	{
		SetPositionInViewport(ScreenPosition, true);
	}
}

void UZfResourceHPWidget::NativeDestruct()
{
	if (ResourceActor)
	{
		if (UZfGatheringComponent* GC = ResourceActor->FindComponentByClass<UZfGatheringComponent>())
		{
			GC->OnResourceHPChanged.RemoveDynamic(this, &UZfResourceHPWidget::BP_OnHPChanged);
		}
	}

	Super::NativeDestruct();
}