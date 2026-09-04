// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeaponHolder.h"
#include "Animation/AnimInstance.h"
#include "ShooterWeapon.generated.h"

class IShooterWeaponHolder;
class AShooterProjectile;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;
class USkeletalMesh;

/**
 *  Base class for a simple first person shooter weapon
 *  Provides both first person and third person perspective meshes
 *  Handles ammo and firing logic
 *  Interacts with the weapon owner through the ShooterWeaponHolder interface
 */
UCLASS(abstract)
class MYPROJECT7_API AShooterWeapon : public AActor
{
	GENERATED_BODY()
	
	/** First person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** Punto rojo del laser, pegado a la superficie apuntada */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* LaserDot;

	/** Haz del laser, desde la boca hasta el punto de impacto */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* LaserBeam;

	/** Third person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonMesh;

protected:

	/** Cast pointer to the weapon owner */
	IShooterWeaponHolder* WeaponOwner;

	/** Type of projectiles this weapon will shoot */
	UPROPERTY(EditAnywhere, Category="Ammo")
	TSubclassOf<AShooterProjectile> ProjectileClass;

	/** Number of bullets in a magazine */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 100))
	int32 MagazineSize = 10;

	/** Number of bullets in the current magazine */
	int32 CurrentBullets = 0;
	
	/** Animation montage to play when firing this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* FiringMontage;

	/** Montage shared by the mannequin rifle arms. Its left-hand IK curve frees
	 *  the hand for the SCAR magazine sequence while the real clip bone moves. */
	UPROPERTY(EditAnywhere, Category="Weapon|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, Category="Weapon|Reload", meta=(ClampMin=0.5, ClampMax=6.0, Units="s"))
	float ReloadDuration = 2.15f;

	/** Sound placed on the weapon during the reload. */
	UPROPERTY(EditAnywhere, Category="Weapon|Reload")
	TObjectPtr<class USoundBase> ReloadSound;

	/** Component-local travel of the SCAR's actual clip bone while reloading. */
	UPROPERTY(EditAnywhere, Category="Weapon|Reload", meta=(ClampMin=1, ClampMax=80, Units="cm"))
	float MagazineTravelDistance = 24.0f;

	/** Lower/turn the rifle while the left hand manipulates the magazine. */
	UPROPERTY(EditAnywhere, Category="Weapon|Reload")
	FVector ReloadWeaponOffset = FVector(-4.0f, 4.0f, -6.0f);

	UPROPERTY(EditAnywhere, Category="Weapon|Reload")
	FRotator ReloadWeaponRotation = FRotator(7.0f, 0.0f, -5.0f);

	/** AnimInstance class to set for the first person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** AnimInstance class to set for the third person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** Cone half-angle for variance while aiming */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float AimVariance = 0.0f;

	/** Amount of firing recoil to apply to the owner */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 0.0f;

	/** Name of the first person muzzle socket where projectiles will spawn */
	UPROPERTY(EditAnywhere, Category="Aim")
	FName MuzzleSocketName;

	// ---- Sonido -----------------------------------------------------------
	/** Estampido del disparo. Se lanza en la boca del cañon. */
	UPROPERTY(EditAnywhere, Category="Weapon|Sonido")
	TObjectPtr<class USoundBase> FireSound;

	/** Volumen del disparo. Un fusil pide mas cuerpo que una pistola. */
	UPROPERTY(EditAnywhere, Category="Weapon|Sonido", meta=(ClampMin=0.1, ClampMax=4.0))
	float FireVolume = 1.6f;

	/** Tono. Por debajo de 1 suena mas grave y mas grande. */
	UPROPERTY(EditAnywhere, Category="Weapon|Sonido", meta=(ClampMin=0.3, ClampMax=2.0))
	float FirePitch = 1.0f;

	// ---- Laser -------------------------------------------------------------
	/** Laser encendido. El punto rojo marca donde va a impactar la bala. */
	UPROPERTY(EditAnywhere, Category="Weapon|Laser")
	bool bLaserEnabled = true;

	/** Dibujar el haz, no solo el punto. */
	UPROPERTY(EditAnywhere, Category="Weapon|Laser")
	bool bLaserBeamVisible = true;

	/** Tamaño del punto en cm a corta distancia. Crece con la distancia para
	 *  que en pantalla se vea siempre parecido. */
	UPROPERTY(EditAnywhere, Category="Weapon|Laser", meta=(ClampMin=0.1, ClampMax=8, Units="cm"))
	float LaserDotSize = 0.5f;

	/** Grosor del haz en cm. */
	UPROPERTY(EditAnywhere, Category="Weapon|Laser", meta=(ClampMin=0.05, ClampMax=2, Units="cm"))
	float LaserBeamThickness = 0.15f;

	/** Alcance maximo del laser. */
	UPROPERTY(EditAnywhere, Category="Weapon|Laser", meta=(ClampMin=100, ClampMax=100000, Units="cm"))
	float LaserRange = 20000.0f;

	/** Desplazamiento del emisor respecto a la boca, en ejes del arma. */
	UPROPERTY(EditAnywhere, Category="Weapon|Laser")
	FVector LaserOriginOffset = FVector::ZeroVector;

	/** Recoloca el punto y el haz cada frame. */
	void UpdateLaser();

	// ---- Trazadora ---------------------------------------------------------
	/** Se ve la bala salir del cañon como un trazo luminoso. */
	UPROPERTY(EditAnywhere, Category="Weapon|Trazadora")
	bool bTracerEnabled = true;

	/** Grosor del trazo en cm. */
	UPROPERTY(EditAnywhere, Category="Weapon|Trazadora", meta=(ClampMin=0.2, ClampMax=6, Units="cm"))
	float TracerThickness = 1.1f;

	/** Cuanto dura visible. Muy corto: es un destello, no una linea fija. */
	UPROPERTY(EditAnywhere, Category="Weapon|Trazadora", meta=(ClampMin=0.01, ClampMax=0.5, Units="s"))
	float TracerLife = 0.06f;

	/** Largo del trazo en cm. No llega hasta el blanco: es un tramo que sale
	 *  disparado de la boca, como una trazadora real. */
	UPROPERTY(EditAnywhere, Category="Weapon|Trazadora", meta=(ClampMin=20, ClampMax=4000, Units="cm"))
	float TracerLength = 900.0f;

	/** Dibuja el trazo entre dos puntos del mundo. */
	void SpawnTracer(const FVector& Desde, const FVector& Hacia);

	/** Lee la cvar  arma.fit  y recoloca la malla en vivo. */
	void ApplyLiveFitFromConsole();

	/** Lee la cvar  arma.yaw  y gira SOLO el arma equipada sobre su eje
	 *  vertical (la culata a un lado, la boca al otro). A diferencia de
	 *  arma.fit, no toca a las armas guardadas: solo obedece la que no
	 *  esta oculta, o sea la que llevas en la mano. */
	void ApplyLiveYawFromConsole();

	/** Lee la cvar  arma.boca  y fija MuzzleLocalOffset (punto de salida de
	 *  bala y trazadora) SOLO en el arma equipada. */
	void ApplyLiveMuzzleFromConsole();

	// ---- Boca del cañon ----------------------------------------------------
	/** Punto de la boca en espacio LOCAL de la malla. Si queda a cero se
	 *  DERIVA de las medidas de la malla (punta del eje largo). */
	UPROPERTY(EditAnywhere, Category="Weapon|Boca")
	FVector MuzzleLocalOffset = FVector::ZeroVector;

	/** Por defecto la boca se calcula en la punta visible del arma. Solo activa
	 *  esto si el asset tiene un socket de boca revisado y correcto. */
	UPROPERTY(EditAnywhere, Category="Weapon|Boca")
	bool bUseAuthoredMuzzleSocket = false;

	/** Igual que el socket: queda apagado para que un offset viejo no mande la
	 *  bala al laser o a una pieza decorativa. */
	UPROPERTY(EditAnywhere, Category="Weapon|Boca")
	bool bUseAuthoredMuzzleOffset = false;

	/** Pequeno avance fuera de la punta del canion para evitar colision visual. */
	UPROPERTY(EditAnywhere, Category="Weapon|Boca", meta=(ClampMin=0, ClampMax=20, Units="cm"))
	float MuzzleTipForwardOffset = 2.5f;

	/** Posicion de MUNDO de la boca. Unico sitio del que salen bala,
	 *  trazadora, sonido y laser, para que los cuatro coincidan. */
	FVector GetMuzzleWorldLocation() const;

	// ---- Encaje fijo -------------------------------------------------------
	/** Colocacion de la malla respecto al socket de la mano. Equivale a lo que
	 *  se calibra con  arma.fit , pero guardado. */
	UPROPERTY(EditAnywhere, Category="Weapon|Encaje")
	FVector WeaponMeshLocation = FVector(4.09f, 12.0f, -5.13f);   // calibrado a mano para el SCAR

	UPROPERTY(EditAnywhere, Category="Weapon|Encaje")
	FRotator WeaponMeshRotation = FRotator(-5.0f, 0.0f, 0.0f);   // punta un poco abajo

	UPROPERTY(EditAnywhere, Category="Weapon|Encaje", meta=(ClampMin=0.05, ClampMax=5))
	float WeaponMeshScale = 1.0f;

	/** Lee la cvar  arma.ads  y recoloca la pose de apuntado en vivo. */
	void ApplyLiveADSFromConsole();

	/** Lee  arma.ocultarhueso  y esconde piezas de la malla del arma. */
	void ApplyLiveHideBonesFromConsole();

	/** Huesos que se ocultan al equipar el arma, separados por comas. Sirve
	 *  para quitar miras o accesorios que vienen dentro de la misma malla.
	 *  Se descubre cual es con  arma.ocultarhueso  y se deja fijo aqui. */
	UPROPERTY(EditAnywhere, Category="Weapon|Malla")
	FString HiddenBonesOnEquip;

	// ---- Postura de los brazos con ESTA arma -------------------------------
	/** Desplazamiento de los BRAZOS cuando se lleva esta arma, en ejes de
	 *  camara: X adelante, Y derecha, Z arriba. Una escopeta se sujeta mas
	 *  baja y pegada al cuerpo que un fusil; esto lo permite por arma. */
	UPROPERTY(EditAnywhere, Category="Weapon|Brazos")
	FVector ArmsHoldOffset = FVector::ZeroVector;

	/** Giro de los brazos con esta arma. */
	UPROPERTY(EditAnywhere, Category="Weapon|Brazos")
	FRotator ArmsHoldRotation = FRotator::ZeroRotator;

public:

	FVector GetArmsHoldOffset() const { return ArmsHoldOffset; }
	FRotator GetArmsHoldRotation() const { return ArmsHoldRotation; }

protected:

	/** Mantiene la malla en su encaje. Se llama cada frame porque
	 *  AttachWeaponMeshes engancha con SnapToTarget y eso pone la
	 *  transformacion relativa a cero: sin esto el encaje se pierde. */
	void ApplyWeaponFit();

	// ---- Pegada y perdigones ----------------------------------------------
	/** Proyectiles por disparo. 1 = normal. Una escopeta dispara 8-12
	 *  perdigones a la vez, cada uno con su desvio. */
	UPROPERTY(EditAnywhere, Category="Weapon|Pegada", meta=(ClampMin=1, ClampMax=24))
	int32 PelletsPerShot = 1;

	/** Dispersion extra de los perdigones, en grados. Se suma a AimVariance. */
	UPROPERTY(EditAnywhere, Category="Weapon|Pegada", meta=(ClampMin=0, ClampMax=30, Units="Degrees"))
	float PelletSpread = 0.0f;

	/** Daño por proyectil. Si es <= 0 manda el del propio proyectil. */
	UPROPERTY(EditAnywhere, Category="Weapon|Pegada", meta=(ClampMin=0, ClampMax=200))
	float ProjectileDamage = 0.0f;

	/** Fuerza fisica del impacto: lo que manda por los aires lo que toque.
	 *  Si es <= 0 manda la del propio proyectil. */
	UPROPERTY(EditAnywhere, Category="Weapon|Pegada", meta=(ClampMin=0, ClampMax=200000))
	float ProjectileForce = 0.0f;

	/**
	 * Construye la transformacion ADS desde el hueso de camara que trae el
	 * asset del SCAR. Asi la mira queda sobre el eje real de la camara, no en
	 * un offset aproximado respecto a la mano derecha.
	 */
	bool GetADSMeshWorldTransform(FTransform& OutTransform) const;

	/** Interpolates hip fire / ADS and layers the reload pose without changing
	 *  attachment sockets. */
	void UpdateWeaponPose(float DeltaSeconds);

	/** Moves the real SCAR clip bone through a continuous removal/insertion. */
	void UpdateReloadMagazineVisual();
	void ClearReloadMagazineVisual();
	bool CacheMagazineBone();
	void FinishReload();

	/** Distance ahead of the muzzle that bullets will spawn at */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 10.0f;

	// ---- ADS ---------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category="Weapon|ADS")
	// Respaldo para armas que no tengan un hueso de camara. El SCAR usa el
	// hueso ADSReferenceBoneName y por eso no depende de este ajuste manual.
	// Calibrado desde la captura real: respecto a HandGrip_R, Y negativo trae
	// el rifle al eje de la camara y Z negativo baja la linea de miras hasta el
	// centro de pantalla. No queda lateral ni demasiado alto.
	FVector ADSMeshLocation = FVector(-11.0f, -12.0f, -10.0f);   // mas a la izquierda, pegada a los brazos

	UPROPERTY(EditAnywhere, Category="Weapon|ADS")
	FRotator ADSMeshRotation = FRotator::ZeroRotator;

	/** Locator exportado con el SCAR para una vista de primera persona real. */
	UPROPERTY(EditAnywhere, Category="Weapon|ADS")
	// El FBX original declara un locator "camera", pero el SkeletalMesh que
	// Unreal genero no lo conserva. Se deja vacio para usar el encaje calibrado
	// anterior en lugar de intentar una referencia inexistente cada frame.
	FName ADSReferenceBoneName = NAME_None;

	/** Ajuste minimo en espacio de camara; cero coloca el ojo directamente
	 * detras de la mira trasera, como una vista ADS militar. */
	UPROPERTY(EditAnywhere, Category="Weapon|ADS")
	FVector ADSEyeOffset = FVector::ZeroVector;

	/** Solo se usa si un asset exporta su locator con una orientacion distinta.
	 * El SCAR importado usa cero: su hueso camera ya mira hacia +X de Unreal. */
	UPROPERTY(EditAnywhere, Category="Weapon|ADS")
	FRotator ADSReferenceRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category="Weapon|ADS", meta=(ClampMin=1, ClampMax=30))
	float ADSInterpolationSpeed = 13.0f;

	bool bIsAiming = false;
	float ADSAlpha = 0.0f;

	/** If true, this weapon will automatically fire at the refire rate */
	UPROPERTY(EditAnywhere, Category="Refire")
	bool bFullAuto = false;

	/** Time between shots for this weapon. Affects both full auto and semi auto modes */
	UPROPERTY(EditAnywhere, Category="Refire", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float RefireRate = 0.5f;

	/** Game time of last shot fired, used to enforce refire rate on semi auto */
	float TimeOfLastShot = 0.0f;

	/** If true, the weapon is currently firing */
	bool bIsFiring = false;
	bool bIsReloading = false;
	float ReloadStartTime = 0.0f;
	FTimerHandle ReloadTimer;
	TArray<FTransform> CachedMagazineRefPose;
	int32 MagazineBoneIndex = INDEX_NONE;
	int32 MagazineParentBoneIndex = INDEX_NONE;
	TObjectPtr<class USkeletalMesh> CachedMagazineMesh;

	/** Timer to handle full auto refiring */
	FTimerHandle RefireTimer;

	/** Cast pawn pointer to the owner for AI perception system interactions */
	TObjectPtr<APawn> PawnOwner;

	/** Loudness of the shot for AI perception system interactions */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100))
	float ShotLoudness = 1.0f;

	/** Max range of shot AI perception noise */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float ShotNoiseRange = 300.0f;

	/** Tag to apply to noise generated by shooting this weapon */
	UPROPERTY(EditAnywhere, Category="Perception")
	FName NoiseOwnerTag = FName("Shot");

public:	

	/** Constructor */
	AShooterWeapon();

protected:
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Called when the weapon's owner is destroyed */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

public:

	/** Activates this weapon and gets it ready to fire */
	void ActivateWeapon(const FName& OwnerTag);

	/** Deactivates this weapon */
	void DeactivateWeapon();

	/** Start firing this weapon */
	void StartFiring();

	/** Stop firing this weapon */
	void StopFiring();

	/** Begins a non-magical reload of the equipped weapon. */
	void StartReload();

	/** Smooth ADS state, driven by right mouse button. */
	void SetAiming(bool bNewAiming);
	bool IsAiming() const { return bIsAiming && !bIsReloading; }

	/** 0 = cadera, 1 = apuntando. Lo lee el personaje para llevar los BRAZOS
	 *  a la pose de apuntado, no solo el arma. */
	float GetADSAlpha() const { return ADSAlpha; }

protected:

	/** Fire the weapon */
	virtual void Fire();

	/** Called when the refire rate time has passed while shooting semi auto weapons */
	void FireCooldownExpired();

	/** Fire a projectile towards the target location */
	virtual void FireProjectile(const FVector& TargetLocation);

	/** Calculates the spawn transform for projectiles shot by this weapon */
	FTransform CalculateProjectileSpawnTransform(const FVector& TargetLocation) const;

public:

	/** Actualiza el laser cada frame */
	virtual void Tick(float DeltaSeconds) override;


	/** Returns the first person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** Returns the third person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** Returns the first person anim instance class */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const;

	/** Returns the third person anim instance class */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const;

	/** Returns the magazine size */
	int32 GetMagazineSize() const { return MagazineSize; };

	/** Returns the current bullet count */
	int32 GetBulletCount() const { return CurrentBullets; }
};
