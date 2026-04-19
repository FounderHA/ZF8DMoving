#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZfResourceHPWidget.generated.h"

UCLASS()
class ZF8DMOVING_API UZfResourceHPWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Chamado pela GA ao criar a widget — passa o ator do recurso
	UFUNCTION(BlueprintCallable, Category = "Zf|ResourceHP")
	void InitResourceWidget(AActor* InResourceActor);

protected:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Chamado pelo timer — atualiza a posição na tela
	void UpdateScreenPosition();

	// Blueprint implementa a atualização do HP
	UFUNCTION(BlueprintImplementableEvent, Category = "Zf|ResourceHP")
	void BP_OnHPChanged(float CurrentHP, float MaxHP);

	// Offset acima do ator — configurável no editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zf|ResourceHP")
	FVector WorldOffset = FVector(0.f, 0.f, 100.f);

private:

	UPROPERTY()
	bool bWasOnScreen = false;
	
	UPROPERTY()
	TObjectPtr<AActor> ResourceActor;
	
};