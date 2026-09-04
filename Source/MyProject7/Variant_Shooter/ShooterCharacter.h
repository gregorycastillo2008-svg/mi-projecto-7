// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyProject7Character.h"
#include "ShooterWeaponHolder.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);

/**
 *  A player controllable first person shooter character
 *  Manages a weapon inventory through the IShooterWeaponHolder interface
 *  Manages health and death
 */
UCLASS(abstract)
class MYPROJECT7_API AShooterCharacter : public AMyProject7Character, public IShooterWeaponHolder
{
	GENERATED_BODY()
	
	/** AI Noise emitter component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

protected:

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Switch weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Current HP remaining to this character */
	float CurrentHP = 0.0f;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** Actor tag to grant this character when it dies */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** Tag to pass to weapons and projectiles to identify their AI perception noise as player-generated */
	UPROPERTY(EditAnywhere, Category="Tags")
	FName PlayerTag = FName("Player");

	/** Arma con la que aparece el jugador. Si se deja vacia, se carga la
	 *  pistola del template. Se puede cambiar desde BP_ShooterCharacter. */
	UPROPERTY(EditAnywhere, Category="Weapons")
	TSubclassOf<AShooterWeapon> StartingWeaponClass;

	/** Colocacion de los brazos de primera persona respecto a su sitio de
	 *  origen. Se calibra en vivo con la cvar  brazos.fit  y luego se deja
	 *  fijo aqui. Mover los brazos mueve tambien el arma, que cuelga de ellos. */
	UPROPERTY(EditAnywhere, Category="Bodycam|Brazos")
	FVector ArmsOffset = FVector::ZeroVector;

	/** Giro de los brazos, en grados. */
	UPROPERTY(EditAnywhere, Category="Bodycam|Brazos")
	FRotator ArmsRotation = FRotator::ZeroRotator;

	/** Tick: aplica el encaje de brazos leido de la consola. */
	virtual void Tick(float DeltaSeconds) override;

	/** Lee la cvar  brazos.fit  y recoloca la malla de brazos. */
	void ApplyLiveArmsFromConsole();

	// ---- Pose de brazos al apuntar -----------------------------------------
	/** Desplazamiento de los BRAZOS al apuntar, en ejes de CAMARA:
	 *  X adelante, Y derecha, Z arriba. Al mover los brazos se mueven tambien
	 *  las dos manos y el arma, que cuelga del socket HandGrip_R. Esa es la
	 *  diferencia con mover solo el arma: aqui la pose entera acompaña. */
	UPROPERTY(EditAnywhere, Category="Aim|ADS")
	FVector ADSArmsOffset = FVector(2.0f, -7.5f, 1.5f);

	/** Giro de los brazos al apuntar. */
	UPROPERTY(EditAnywhere, Category="Aim|ADS")
	FRotator ADSArmsRotation = FRotator::ZeroRotator;

	/** Suavizado de la transicion cadera <-> apuntado. */
	UPROPERTY(EditAnywhere, Category="Aim|ADS", meta=(ClampMin=1, ClampMax=30))
	float ADSArmsSpeed = 11.0f;

	/** Lleva los brazos (y con ellos manos y arma) a la pose de apuntado. */
	void UpdateADSArms(float DeltaSeconds);

	/** Devuelve la vista al punto de origen tras el culatazo. */
	void RecoverRecoil(float DeltaSeconds);

	/** Enciende y apaga la linterna (tecla F). */
	void ToggleFlashlight();

	/** Lee la cvar  brazos.ads  y recoloca la pose de apuntado de brazos. */
	void ApplyLiveADSArmsFromConsole();

	/** Reposo de la malla de brazos, capturado la primera vez. */
	FVector ArmsRestLocation = FVector::ZeroVector;
	FRotator ArmsRestRotation = FRotator::ZeroRotator;
	bool bArmsRestCaptured = false;
	float ArmsADSAlpha = 0.0f;

	/** List of weapons picked up by the character */
	TArray<AShooterWeapon*> OwnedWeapons;

	/** Weapon currently equipped and ready to shoot with */
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

public:

	/** Constructor */
	AShooterCharacter();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** Handle incoming damage */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** Handles aim inputs from either controls or UI interfaces */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward)  override;

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart()  override;

	/** Handles jump end inputs from either controls or UI interfaces */
	virtual void DoJumpEnd()  override;

	/** Handles start firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** Handles stop firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** Handles switch weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	/** Starts the tactical reload for the equipped weapon (R). */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoReload();

	/** Enters and leaves the weapon's smooth aim-down-sights pose. */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartADS();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopADS();

public:

	//~Begin IShooterWeaponHolder interface

	/** Attaches a weapon's meshes to the owner */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;

	/** Plays the firing montage for the weapon */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** Applies weapon recoil to the owner */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** Updates the weapon's HUD with the current ammo count */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** Calculates and returns the aim location for the weapon */
	virtual FVector GetWeaponTargetLocation() override;

	/** Gives a weapon of this class to the owner */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;

	/** Activates the passed weapon */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;

	/** Deactivates the passed weapon */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;

	/** Notifies the owner that the weapon cooldown has expired and it's ready to shoot again */
	virtual void OnSemiWeaponRefire() override;

	//~End IShooterWeaponHolder interface

protected:

	/** Returns true if the character already owns a weapon of the given class */
	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** Called when this character's HP is depleted */
	void Die();

	/** Called to allow Blueprint code to react to this character's death */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** Called from the respawn timer to destroy this character and force the PC to respawn */
	void OnRespawn();

public:

	/** Returns true if the character is dead */
	bool IsDead() const;

	/** Used by the bodycam manager to stabilize the image while aiming. */
	bool IsAimingDownSights() const;

	/** Sets the team ID for this character */
	void SetTeam(uint8 Team);
};
