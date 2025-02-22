// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ABItemBox.generated.h"

UCLASS()
class ARENABATTLE_API AABItemBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AABItemBox();

	// StageGimmick에서 Item Spawn을 위해 Trigger에 대한 Getter 추가
	FORCEINLINE class UBoxComponent* GetTrigger() { return Trigger; }

protected:
	// 아이템 박스가 초기화됐을 때, 에셋 매니저의 목록을 순회해 랜덤하게 1개를 선택해 할당하는 함수
	virtual void PostInitializeComponents() override;

protected:
	// 오버랩 이벤트를 감지하기 위한 델리게이트가 선언되어 있음 -> 함수 연결
	UPROPERTY(VisibleAnywhere, Category=Box)
	TObjectPtr<class UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category=Box)
	TObjectPtr<class UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category=Effect)
	TObjectPtr<class UParticleSystemComponent> Effect;

	UPROPERTY(EditAnywhere, Category=Item)
	TObjectPtr<class UABItemData> Item;  // 부모 클래스 타입을 지정했기 때문에, 어느 아이템이든 들어올 수 있음.

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnEffectFinished(class UParticleSystemComponent* ParticleSystem);

	// TODO: 아이템(미들웨어) 습득(캐릭터: 게임로직) 시 인터페이스를 통해 알림을 전달하도록 설정
};
