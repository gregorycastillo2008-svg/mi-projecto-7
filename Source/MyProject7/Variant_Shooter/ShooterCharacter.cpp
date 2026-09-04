// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "Components/SpotLightComponent.h"

// Colocacion de los brazos en vivo, sin recompilar:
//     brazos.fit <X> <Y> <Z> [pitch] [yaw] [roll]
// Ejes de la malla de brazos. Mover los brazos mueve el arma con ellos.
// Pose de los BRAZOS al apuntar, en vivo:
//     brazos.ads <adelante> <derecha> <arriba> [pitch] [yaw] [roll]
// Negativo en el segundo numero = hacia la izquierda, al centro de pantalla.
static TAutoConsoleVariable<FString> CVarBrazosADS(
	TEXT("brazos.ads"),
	TEXT(""),
	TEXT("Pose de los brazos al apuntar: <adelante> <derecha> <arriba> [pitch] [yaw] [roll]"),
	ECVF_Default);

static TAutoConsoleVariable<FString> CVarBrazosFit(
	TEXT("brazos.fit"),
	TEXT(""),
	TEXT("Coloca los brazos de primera persona: <X> <Y> <Z> [pitch] [yaw] [roll]"),
	ECVF_Default);

AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	// Giro controlado para que el cuerpo no cambie de direccion como un robot.
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD
	OnDamaged.Broadcast(1.0f);

	// --- Arma inicial ------------------------------------------------------
	// El template deja al jugador con las manos vacias: las armas se recogen
	// de los pickups del nivel. Aqui se le da una desde el principio.
	// Se carga en BeginPlay y no en el constructor porque es un Blueprint:
	// con ConstructorHelpers el CDO puede abortar al arrancar.
	// Las tres del template. El orden es el de las teclas 1/2/3.
	// SOLO EL SCAR. La pistola y el lanzagranadas quedan fuera del inventario;
	// sus Blueprints siguen en el proyecto por si se quieren recuperar.
	static const TCHAR* Arsenal[] = {
		TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Rifle.BP_ShooterWeapon_Rifle_C"),
		TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol.BP_ShooterWeapon_Pistol_C"),
		// La escopeta va montada sobre el Blueprint del lanzagranadas: MCP no
		// deja duplicar assets, y ese estaba sin usar. Malla, encaje, boca y
		// stats son los de una Remington 870 de calibre 12.
		TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_GrenadeLauncher.BP_ShooterWeapon_GrenadeLauncher_C"),
	};

	int32 Dadas = 0;
	for (const TCHAR* Ruta : Arsenal)
	{
		if (UClass* C = LoadClass<AShooterWeapon>(nullptr, Ruta))
		{
			AddWeaponClass(C);
			++Dadas;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMA] no se pudo cargar %s"), Ruta);
		}
	}

	// Si el Blueprint del personaje fija un arma concreta, esa manda y se
	// equipa la ultima (queda activa).
	if (StartingWeaponClass)
	{
		AddWeaponClass(StartingWeaponClass);
	}

	UE_LOG(LogTemp, Warning, TEXT("[ARMA] inventario inicial: %d armas"), Dadas);
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
	}

	// El template no traia acciones Enhanced Input para recargar ni ADS. Estas
	// ligaduras directas mantienen las acciones existentes y dan R / clic derecho
	// al jugador sin modificar el mapeo de armas, disparo ni cambio de arma.
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AShooterCharacter::DoReload);
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AShooterCharacter::ToggleFlashlight);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AShooterCharacter::DoStartADS);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AShooterCharacter::DoStopADS);

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1 && !IsDead())
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon(PlayerTag);
	}
}

void AShooterCharacter::DoReload()
{
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartReload();
	}
}

void AShooterCharacter::DoStartADS()
{
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->SetAiming(true);
	}
}

void AShooterCharacter::DoStopADS()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->SetAiming(false);
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	if (Montage && GetFirstPersonMesh())
	{
		if (UAnimInstance* ArmsAnim = GetFirstPersonMesh()->GetAnimInstance())
		{
			ArmsAnim->Montage_Play(Montage, 1.0f);
		}
	}
}

// Escala global del retroceso, en vivo:  arma.recoil 0   lo apaga del todo.
static TAutoConsoleVariable<float> CVarRecoilEscala(
	TEXT("arma.recoil"),
	1.0f,
	TEXT("Escala del retroceso de la vista. 0 = sin retroceso."),
	ECVF_Default);

// Estado del retroceso. Solo vertical: es lo que queda por devolver en pitch.
static float GRecoilDeuda = 0.0f;

// Linterna. Se crea la primera vez que se pulsa F y se cuelga de la camara,
// asi apunta siempre a donde miras y sobrevive al cambio de arma. El puntero
// va a nivel de fichero para no cambiar el layout de la clase (Live Coding).
static TWeakObjectPtr<USpotLightComponent> GLinterna;
static bool GLinternaEncendida = false;

// Tope DURO de lo que puede desviarse la vista. Sin esto, disparar seguido
// acumulaba sin limite y la camara acababa mirando al suelo.
static constexpr float GRecoilTopeGrados = 7.0f;   // tope duro: nunca mas de esto

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// GUARDA: solo el jugador local. El estado es de fichero, asi que sin esto
	// un NPC disparando movia la vista del jugador.
	if (!IsLocallyControlled()) { return; }

	const float Escala = FMath::Max(CVarRecoilEscala.GetValueOnGameThread(), 0.0f);
	// El culatazo por disparo se acota: da igual que un arma traiga 3.2 o 20.
	// Culatazo por disparo. Acotado, pero con margen para que una escopeta
	// pegue de verdad: con FiringRecoil 3.5 sale un golpe seco y visible.
	const float Fuerza = FMath::Min(FMath::Abs(Recoil), 5.0f) * 0.38f * Escala;
	if (Fuerza <= KINDA_SMALL_NUMBER) { return; }

	// Solo se aplica lo que quepa bajo el tope: nunca se acumula sin freno.
	const float Sitio = FMath::Max(GRecoilTopeGrados - GRecoilDeuda, 0.0f);
	const float Sube = FMath::Min(Fuerza, Sitio);
	if (Sube > KINDA_SMALL_NUMBER)
	{
		AddControllerPitchInput(-Sube);
		GRecoilDeuda += Sube;
	}

	// SIN empujon lateral. El yaw de retroceso arrastraba la vista a un lado
	// porque su recuperacion iba atada al ritmo del pitch. El usuario solo
	// pidio un culatazo VERTICAL fuerte, asi que el retroceso es solo vertical.
}

void AShooterCharacter::RecoverRecoil(float DeltaSeconds)
{
	if (!IsLocallyControlled()) { return; }

	// Recuperacion rapida: la vista vuelve al punto de origen en ~0.2 s.
	// Recuperacion. Rapida al principio y suave al final, para que el golpe
	// se sienta seco y la vuelta no parezca mecanica.
	const float Paso = FMath::Max(GRecoilDeuda * 5.0f, 1.2f) * DeltaSeconds;

	if (GRecoilDeuda > KINDA_SMALL_NUMBER)
	{
		const float D = FMath::Min(GRecoilDeuda, Paso);
		AddControllerPitchInput(D);
		GRecoilDeuda -= D;
	}
	else
	{
		GRecoilDeuda = 0.0f;
	}
}

bool AShooterCharacter::IsAimingDownSights() const
{
	return CurrentWeapon && CurrentWeapon->IsAiming();
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon(PlayerTag);
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

void AShooterCharacter::SetTeam(uint8 Team)
{
	TeamByte = Team;
}

// ============================================================================
//  BRAZOS EN VIVO
//  brazos.fit <X> <Y> <Z> [pitch] [yaw] [roll]
//  El arma cuelga del socket HandGrip_R de esta malla, asi que moviendo los
//  brazos se mueve el conjunto y el arma no se despega de las manos.
// ============================================================================
void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyLiveArmsFromConsole();
	ApplyLiveADSArmsFromConsole();
	UpdateADSArms(DeltaSeconds);
	RecoverRecoil(DeltaSeconds);
}

void AShooterCharacter::ApplyLiveArmsFromConsole()
{
	static FString Ultimo;
	const FString Linea = CVarBrazosFit.GetValueOnGameThread();
	if (Linea == Ultimo) { return; }
	Ultimo = Linea;
	if (Linea.IsEmpty()) { return; }

	TArray<FString> P;
	Linea.ParseIntoArrayWS(P);
	if (P.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BRAZOS] uso: brazos.fit <X> <Y> <Z> [pitch] [yaw] [roll]"));
		return;
	}

	ArmsOffset = FVector(FCString::Atof(*P[0]), FCString::Atof(*P[1]), FCString::Atof(*P[2]));
	if (P.Num() >= 6)
	{
		ArmsRotation = FRotator(FCString::Atof(*P[3]), FCString::Atof(*P[4]), FCString::Atof(*P[5]));
	}

	if (USkeletalMeshComponent* Brazos = GetFirstPersonMesh())
	{
		Brazos->SetRelativeLocationAndRotation(ArmsOffset, ArmsRotation);
		UE_LOG(LogTemp, Warning, TEXT("[BRAZOS] -> loc %s  rot %s"),
			*ArmsOffset.ToCompactString(), *ArmsRotation.ToCompactString());
	}
}

// ============================================================================
//  POSE DE BRAZOS AL APUNTAR
//  Mover solo el arma la despega de las manos. Aqui se mueve la MALLA DE
//  BRAZOS: las dos manos y el arma -que cuelga del socket HandGrip_R- van con
//  ella, asi que la pose entera acompaña como en un FPS de verdad.
//
//  La camara cuelga del hueso "head" de esta misma malla. Si se moviera con
//  ella, la vista se desplazaria y la mira dejaria de coincidir con el centro.
//  Por eso al final se le devuelve su posicion de mundo: la vista queda
//  clavada y el disparo -que traza desde la camara- no se desvia.
// ============================================================================
void AShooterCharacter::UpdateADSArms(float DeltaSeconds)
{
	USkeletalMeshComponent* Brazos = GetFirstPersonMesh();
	UCameraComponent* Cam = GetFirstPersonCameraComponent();
	if (!Brazos || !Cam) { return; }

	// Reposo de la camara RESPECTO A SU PADRE. A nivel de fichero para no tocar
	// la cabecera y que Live Coding siga valiendo. Un jugador, un personaje.
	static FVector CamRestRelative = FVector::ZeroVector;
	static bool bCamRestCaptured = false;

	if (!bArmsRestCaptured)
	{
		ArmsRestLocation = Brazos->GetRelativeLocation();
		ArmsRestRotation = Brazos->GetRelativeRotation();
		bArmsRestCaptured = true;
	}
	if (!bCamRestCaptured)
	{
		CamRestRelative = Cam->GetRelativeLocation();
		bCamRestCaptured = true;
	}

	const float Objetivo = CurrentWeapon ? CurrentWeapon->GetADSAlpha() : 0.0f;
	ArmsADSAlpha = FMath::FInterpTo(ArmsADSAlpha, Objetivo, DeltaSeconds, ADSArmsSpeed);

	// 1) TODO a reposo, brazos Y CAMARA.
	//
	//    EL FALLO QUE HABIA: solo se reseteaban los brazos y luego se corregia
	//    la camara con SetWorldLocation. Como la camara cuelga de los brazos,
	//    esa correccion quedaba guardada como offset relativo y se SUMABA al
	//    frame siguiente. Por eso la vista derivaba sin parar hacia abajo y a
	//    la izquierda. Reseteando tambien la camara, no hay acumulacion.
	Brazos->SetRelativeLocationAndRotation(ArmsRestLocation, ArmsRestRotation);
	Cam->SetRelativeLocation(CamRestRelative);

	// 2) Donde debe quedarse la vista: justo donde esta ahora, en reposo.
	const FVector AnclaMundo = Cam->GetComponentLocation();
	const FQuat RotCamara = Cam->GetComponentQuat();

	// 3) Hip fire vivo: una respiracion muy contenida en reposo y un sway que
	// aumenta suavemente al caminar/correr. Se aplica a la MALLA COMPLETA de
	// brazos; por tanto el arma, mano derecha y mano izquierda permanecen como
	// un conjunto fisico y no como piezas independientes.
	const float Velocidad = GetVelocity().Size2D();
	const float VelocidadMax = FMath::Max(GetCharacterMovement()->GetMaxSpeed(), 1.0f);
	const float Movimiento = FMath::Clamp(Velocidad / VelocidadMax, 0.0f, 1.0f);
	const float Tiempo = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float SwayAlpha = 1.0f - ArmsADSAlpha;
	const float Respiracion = Tiempo * 1.65f;
	const float Paso = Tiempo * FMath::Lerp(2.0f, 7.0f, Movimiento);

	const FVector IdleEnCamara(
		FMath::Sin(Respiracion) * 0.12f,
		FMath::Sin(Respiracion * 0.73f) * 0.16f + FMath::Sin(Paso) * 0.25f * Movimiento,
		FMath::Cos(Respiracion * 2.0f) * 0.10f + FMath::Abs(FMath::Sin(Paso)) * 0.32f * Movimiento);
	const FRotator IdleRotacion(
		FMath::Sin(Paso) * 0.35f * Movimiento,
		FMath::Sin(Respiracion * 0.80f) * 0.28f + FMath::Sin(Paso * 0.5f) * 0.50f * Movimiento,
		FMath::Sin(Paso) * 0.28f * Movimiento);

	// En ADS el sway se desvanece para mantener la mira estable. El offset ADS
	// mueve tambien los brazos; no se desplaza solo la malla del rifle.
	// POSTURA POR ARMA. Cada arma dice como quiere que se sujeten los brazos:
	// una escopeta se lleva mas baja y pegada al cuerpo que un fusil. Se suma
	// a lo demas, asi que convive con el apuntado y el sway.
	FVector PorArma = FVector::ZeroVector;
	FRotator GiroPorArma = FRotator::ZeroRotator;
	if (CurrentWeapon)
	{
		PorArma = CurrentWeapon->GetArmsHoldOffset();
		GiroPorArma = CurrentWeapon->GetArmsHoldRotation();
	}

	const FVector EnCamara = PorArma + ADSArmsOffset * ArmsADSAlpha + IdleEnCamara * SwayAlpha;
	const FRotator RotacionObjetivo = ArmsRestRotation + GiroPorArma
		+ ADSArmsRotation * ArmsADSAlpha + IdleRotacion * SwayAlpha;

	// 4) Mover los BRAZOS. Las dos manos y el arma van con ellos.
	USceneComponent* Padre = Brazos->GetAttachParent();
	const FQuat RotPadre = Padre ? Padre->GetComponentQuat() : FQuat::Identity;
	const FVector Delta = RotPadre.UnrotateVector(RotCamara.RotateVector(EnCamara));

	Brazos->SetRelativeLocationAndRotation(
		ArmsRestLocation + Delta,
		RotacionObjetivo);

	// 5) Devolver la camara a su ancla. Se corrige por DIFERENCIA sobre la
	//    posicion de reposo, que acabamos de restaurar: nunca se acumula.
	const FVector Desplazada = Cam->GetComponentLocation();
	Cam->AddWorldOffset(AnclaMundo - Desplazada, false, nullptr, ETeleportType::TeleportPhysics);
}

void AShooterCharacter::ApplyLiveADSArmsFromConsole()
{
	static FString Ultimo;
	const FString Linea = CVarBrazosADS.GetValueOnGameThread();
	if (Linea == Ultimo || Linea.IsEmpty()) { return; }
	Ultimo = Linea;

	TArray<FString> P;
	Linea.ParseIntoArrayWS(P);
	if (P.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ADS] uso: brazos.ads <adelante> <derecha> <arriba> [pitch] [yaw] [roll]"));
		return;
	}

	ADSArmsOffset = FVector(FCString::Atof(*P[0]), FCString::Atof(*P[1]), FCString::Atof(*P[2]));
	if (P.Num() >= 6)
	{
		ADSArmsRotation = FRotator(FCString::Atof(*P[3]), FCString::Atof(*P[4]), FCString::Atof(*P[5]));
	}
	UE_LOG(LogTemp, Warning, TEXT("[ADS] brazos -> %s  rot %s"),
		*ADSArmsOffset.ToCompactString(), *ADSArmsRotation.ToCompactString());
}

// ============================================================================
//  LINTERNA (tecla F)
//  Va colgada de la CAMARA, no del arma: asi alumbra siempre a donde miras y
//  no se apaga ni se recoloca al cambiar de arma.
//
//  OJO con la movilidad: una luz nace como Stationary y NO ilumina si su padre
//  se mueve. Ponerla en Movable es lo que hace que funcione de verdad.
// ============================================================================
void AShooterCharacter::ToggleFlashlight()
{
	UCameraComponent* Cam = GetFirstPersonCameraComponent();
	if (!Cam) { return; }

	if (!GLinterna.IsValid())
	{
		USpotLightComponent* Luz = NewObject<USpotLightComponent>(this, TEXT("Linterna"));
		Luz->SetMobility(EComponentMobility::Movable);          // sin esto no alumbra
		Luz->SetupAttachment(Cam);
		Luz->RegisterComponent();
		Luz->AttachToComponent(Cam, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Luz->SetRelativeLocation(FVector(10.0f, 6.0f, -4.0f));   // algo adelante y a un lado
		Luz->SetIntensityUnits(ELightUnits::Candelas);
		Luz->SetIntensity(24000.0f);
		Luz->SetAttenuationRadius(4500.0f);
		Luz->SetInnerConeAngle(16.0f);
		Luz->SetOuterConeAngle(34.0f);
		Luz->SetLightColor(FLinearColor(1.0f, 0.96f, 0.88f));    // blanco calido, no azul
		Luz->SetCastShadows(true);
		Luz->SetVisibility(false);
		GLinterna = Luz;
		GLinternaEncendida = false;
	}

	GLinternaEncendida = !GLinternaEncendida;
	GLinterna->SetVisibility(GLinternaEncendida);
	UE_LOG(LogTemp, Warning, TEXT("[LINTERNA] %s"), GLinternaEncendida ? TEXT("encendida") : TEXT("apagada"));
}
