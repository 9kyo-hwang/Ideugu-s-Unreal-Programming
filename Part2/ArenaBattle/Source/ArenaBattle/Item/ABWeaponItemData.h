// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameData/ABCharacterStat.h"
#include "Item/ABItemData.h"
#include "ABWeaponItemData.generated.h"

/**
 * 
 */
UCLASS()
class ARENABATTLE_API UABWeaponItemData : public UABItemData
{
	GENERATED_BODY()

public:
	UABWeaponItemData();
	
	UPROPERTY(EditAnywhere, Category=Weapon)
	TSoftObjectPtr<class USkeletalMesh> WeaponMesh;  // Soft Referencing

	// 아이템 종류 무관하게 같은 태그를 가지도록 설정
	FORCEINLINE virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("ABItemData", GetFName());
	}

	UPROPERTY(EditAnywhere, Category=Stat)
	FABCharacterStat ModifierStat;
};