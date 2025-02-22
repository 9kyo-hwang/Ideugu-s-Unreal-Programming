// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABGASCharacterPlayer.h"
#include "AbilitySystemComponent.h"
#include "Player/ABGASPlayerState.h"
#include "EnhancedInputComponent.h"
#include "UI/ABGASWidgetComponent.h"
#include "UI/ABGASUserWidget.h"
#include "Attribute/ABCharacterAttributeSet.h"
#include "Tag/ABGameplayTag.h"

AABGASCharacterPlayer::AABGASCharacterPlayer()
{
	ASC = nullptr;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ComboActionMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattleGAS/Animation/AM_ComboAttack.AM_ComboAttack'"));
	if (ComboActionMontageRef.Object)
	{
		ComboActionMontage = ComboActionMontageRef.Object;
	}

	HpBar = CreateDefaultSubobject<UABGASWidgetComponent>(TEXT("Widget"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(TEXT("/Game/ArenaBattle/UI/WBP_HpBar.WBP_HpBar_C"));
	if (HpBarWidgetRef.Class)
	{
		HpBar->SetWidgetClass(HpBarWidgetRef.Class);
		HpBar->SetWidgetSpace(EWidgetSpace::Screen);
		HpBar->SetDrawSize(FVector2D(200.0f, 20.f));
		HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/InfinityBladeWeapons/Weapons/Blunt/Blunt_Hellhammer/SK_Blunt_HellHammer.SK_Blunt_HellHammer'"));
	if (WeaponMeshRef.Object)
	{
		WeaponMesh = WeaponMeshRef.Object;
	}

	WeaponRange = 75.f;
	WeaponAttackRate = 100.0f;

	// 몽타주 에셋 로딩
	static ConstructorHelpers::FObjectFinder<UAnimMontage> SkillActionMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattleGAS/Animation/AM_SkillAttack.AM_SkillAttack'"));
	if(SkillActionMontageRef.Object)
	{
		SkillActionMontage = SkillActionMontageRef.Object;
	}
}

UAbilitySystemComponent* AABGASCharacterPlayer::GetAbilitySystemComponent() const
{
	return ASC;
}

void AABGASCharacterPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AABGASPlayerState* GASPS = GetPlayerState<AABGASPlayerState>();
	if (GASPS)
	{
		ASC = GASPS->GetAbilitySystemComponent();
		ASC->InitAbilityActorInfo(GASPS, this);

		ASC->GenericGameplayEventCallbacks.FindOrAdd(ABTAG_EVENT_CHARACTER_WEAPONEQUIP).AddUObject(this, &AABGASCharacterPlayer::EquipWeapon);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(ABTAG_EVENT_CHARACTER_WEAPONUNEQUIP).AddUObject(this, &AABGASCharacterPlayer::UnequipWeapon);

		const UABCharacterAttributeSet* CurrentAttributeSet = ASC->GetSet<UABCharacterAttributeSet>();
		if (CurrentAttributeSet)
		{
			CurrentAttributeSet->OnOutOfHealth.AddDynamic(this, &ThisClass::OnOutOfHealth);
		}

		for (const auto& StartAbility : StartAbilities)
		{
			FGameplayAbilitySpec StartSpec(StartAbility);
			ASC->GiveAbility(StartSpec);
		}

		for (const auto& StartInputAbility : StartInputAbilities)
		{
			FGameplayAbilitySpec StartSpec(StartInputAbility.Value);
			StartSpec.InputID = StartInputAbility.Key;
			ASC->GiveAbility(StartSpec);
		}

		SetupGASInputComponent();

		APlayerController* PlayerController = CastChecked<APlayerController>(NewController);
		PlayerController->ConsoleCommand(TEXT("showdebug abilitysystem"));
	}
}

void AABGASCharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	SetupGASInputComponent();
}

void AABGASCharacterPlayer::SetupGASInputComponent()
{
	if (IsValid(ASC) && IsValid(InputComponent))
	{
		UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AABGASCharacterPlayer::GASInputPressed, 0);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AABGASCharacterPlayer::GASInputReleased, 0);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AABGASCharacterPlayer::GASInputPressed, 1);
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Triggered, this, &AABGASCharacterPlayer::GASInputPressed, 2);  // 스킬 키 입력 시에 대한 바인딩
	}
}

void AABGASCharacterPlayer::GASInputPressed(int32 InputId)
{
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromInputID(InputId);
	if (Spec)
	{
		Spec->InputPressed = true;
		if (Spec->IsActive())
		{
			ASC->AbilitySpecInputPressed(*Spec);
		}
		else
		{
			ASC->TryActivateAbility(Spec->Handle);
		}
	}
}

void AABGASCharacterPlayer::GASInputReleased(int32 InputId)
{
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromInputID(InputId);
	if (Spec)
	{
		Spec->InputPressed = false;
		if (Spec->IsActive())
		{
			ASC->AbilitySpecInputReleased(*Spec);
		}
	}
}

void AABGASCharacterPlayer::OnOutOfHealth()
{
	SetDead();
}

void AABGASCharacterPlayer::EquipWeapon(const FGameplayEventData* EventData)
{
	if (Weapon)
	{
		Weapon->SetSkeletalMesh(WeaponMesh);

		// 무기 장착 시 스킬 부여
		FGameplayAbilitySpec NewSkillSpec(SkillAbilityClass);
		NewSkillSpec.InputID = 2;  // 입력을 부여해줘야 함

		// ASC에서 SkillAbilityClass를 찾지 못했다면
		if(!ASC->FindAbilitySpecFromClass(SkillAbilityClass))
		{
			// 어빌리티가 부여되지 않은 것이므로 GiveAbility
			ASC->GiveAbility(NewSkillSpec);
		}

		const float CurrentAttackRange = ASC->GetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRangeAttribute());
		const float CurrentAttackRate = ASC->GetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRateAttribute());

		ASC->SetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRangeAttribute(), CurrentAttackRange + WeaponRange);
		ASC->SetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRateAttribute(), CurrentAttackRate + WeaponAttackRate);
	}
}

void AABGASCharacterPlayer::UnequipWeapon(const FGameplayEventData* EventData)
{
	if (Weapon)
	{
		const float CurrentAttackRange = ASC->GetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRangeAttribute());
		const float CurrentAttackRate = ASC->GetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRateAttribute());

		ASC->SetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRangeAttribute(), CurrentAttackRange - WeaponRange);
		ASC->SetNumericAttributeBase(UABCharacterAttributeSet::GetAttackRateAttribute(), CurrentAttackRate - WeaponAttackRate);

		// 무기 탈착 시 스킬 해제. 찾았을 때 존재한다면 Clear
		if(FGameplayAbilitySpec* SkillAbilitySpec = ASC->FindAbilitySpecFromClass(SkillAbilityClass))
		{
			ASC->ClearAbility(SkillAbilitySpec->Handle);
		}
		
		Weapon->SetSkeletalMesh(nullptr);
	}
}
