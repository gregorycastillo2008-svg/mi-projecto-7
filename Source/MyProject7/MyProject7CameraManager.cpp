// Copyright Epic Games, Inc. All Rights Reserved.


#include "MyProject7CameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "MyProject7Character.h"
#include "Variant_Shooter/ShooterCharacter.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

// Ajuste de exposicion en caliente, sin recompilar:
//     bodycam.exposure <bias> <minEV> <maxEV>
// Ajuste de la lente en caliente:
//     bodycam.lens <distorsion> <inicioBorde> <finBorde> <vineta>
// Correccion de color en caliente:
//     bodycam.color <saturacion> <contraste>
// Estado del LEAN (asomarse por esquinas). Va a nivel de fichero y no como
// miembro de la clase para no cambiar su layout: asi Live Coding (Ctrl+Alt+F11)
// sigue valiendo. Es un juego de un jugador local, un unico estado basta.
static float   GLeanActual = 0.0f;                 // -1 izquierda .. +1 derecha
static FVector GLeanOffsetPrevio = FVector::ZeroVector;
static constexpr float GLeanVelocidad   = 11.0f;   // rapido pero sin tirones
static constexpr float GLeanMargenPared = 14.0f;   // cm que se respetan al muro

static TAutoConsoleVariable<FString> CVarBodycamLens(
	TEXT("bodycam.lens"),
	TEXT(""),
	TEXT("Lente: <distorsion> <inicioBorde> <finBorde> <vineta>"),
	ECVF_Cheat);

static TAutoConsoleVariable<FString> CVarBodycamExposure(
	TEXT("bodycam.exposure"),
	TEXT(""),
	TEXT("Exposicion de la bodycam: <bias> <minEV> <maxEV>. En EV100."),
	ECVF_Cheat);

static TAutoConsoleVariable<FString> CVarBodycamColor(
	TEXT("bodycam.color"),
	TEXT(""),
	TEXT("Color de la bodycam: <saturacion> <contraste>. 1.0 = neutro."),
	ECVF_Cheat);

AMyProject7CameraManager::AMyProject7CameraManager()
{
	// set the min/max pitch
	ViewPitchMin = -70.0f;
	ViewPitchMax = 80.0f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LensMat(
		TEXT("/Game/BODYCAM_VFX/PP_BodycamLens"));
	if (LensMat.Succeeded()) { BodycamLensMaterial = LensMat.Object; }

	// Se reutiliza una textura de polvo ya incluida en el proyecto como mascara
	// de bloom. Con intensidad muy baja solo aparece alrededor de luces fuertes.
	static ConstructorHelpers::FObjectFinder<UTexture> DirtMask(
		TEXT("/Game/Scene_Warehouse/Assets/MS/3D/Ind_Aba_Box_Dusty_Metal_Pristine_01/T_Ind_Aba_Box_Dusty_Metal_Pristine_01_D"));
	if (DirtMask.Succeeded()) { LensDirtMask = DirtMask.Object; }
}

void AMyProject7CameraManager::AddShotImpulse(float Strength)
{
	const float S = FMath::Clamp(Strength, 0.15f, 2.5f);
	ShotImpulse.Pitch = FMath::Clamp(ShotImpulse.Pitch - Bodycam.ShotPitch * S, -2.0f, 0.0f);
	ShotImpulse.Yaw = FMath::Clamp(ShotImpulse.Yaw + FMath::FRandRange(-Bodycam.ShotYaw, Bodycam.ShotYaw) * S, -0.8f, 0.8f);
	ShotImpulse.Roll = FMath::Clamp(ShotImpulse.Roll + FMath::FRandRange(-Bodycam.ShotRoll, Bodycam.ShotRoll) * S, -1.0f, 1.0f);
}

void AMyProject7CameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// POV base (camara del personaje, control rotation, etc.)
	Super::UpdateViewTarget(OutVT, DeltaTime);

	if (Bodycam.bBodycamEnabled && DeltaTime > 0.0f)
	{
		ApplyBodycam(OutVT, DeltaTime);
	}
}

void AMyProject7CameraManager::ApplyBodycam(FTViewTarget& OutVT, float DeltaTime)
{
	ACharacter* Char = Cast<ACharacter>(OutVT.Target);
	if (!Char)
	{
		Char = PCOwner ? Cast<ACharacter>(PCOwner->GetPawn()) : nullptr;
	}
	if (!Char) { return; }

	const UCharacterMovementComponent* Move = Char->GetCharacterMovement();
	if (!Move) { return; }

	// ------------------------------------------------------------ estado ----
	const FVector Velocity = Char->GetVelocity();
	const float Speed2D = Velocity.Size2D();
	const float MaxSpeed = FMath::Max(Move->MaxWalkSpeed, 1.0f);
	float RawSpeedAlpha = FMath::Clamp(Speed2D / MaxSpeed, 0.0f, 1.0f);
	const bool  bGrounded = !Move->IsFalling();

	// Fallback para personajes que no usan el perfil tactico.
	float SprintAlpha = FMath::Clamp((Speed2D - MaxSpeed * 0.78f) / FMath::Max(MaxSpeed * 0.22f, 1.0f), 0.0f, 1.0f);

	// Suaviza arranques/paradas para que el bob no aparezca de golpe. La
	// velocidad real sigue gobernando la fase; solo se filtra la amplitud.
	SmoothedSpeedAlpha = FMath::FInterpTo(SmoothedSpeedAlpha, RawSpeedAlpha, DeltaTime, 7.0f);
	const float SpeedAlpha = SmoothedSpeedAlpha;

	// Cansancio visual: sube rapido al correr y baja lentamente al parar. No es
	// stamina de gameplay y no modifica movimiento, precision ni armas.
	const float ExertionTarget = (bGrounded ? SprintAlpha : 0.0f);
	const float ExertionInterpSpeed = ExertionTarget > Exertion
		? Bodycam.ExertionBuildSpeed
		: Bodycam.ExertionRecoverySpeed;
	Exertion = FMath::FInterpTo(Exertion, ExertionTarget, DeltaTime, ExertionInterpSpeed);

	// ADS: al apuntar se estabiliza el POV y se interpola al FOV de la mira.
	// Este estado no modifica la trayectoria: el arma sigue trazando hacia el
	// mismo punto de la camara, ahora visualmente alineado con la mira.
	// APUNTADO. Antes solo se detectaba en AShooterCharacter; el pawn del pack
	// (BP_Player, Blueprint puro) no es de esa clase, asi que el ADS nunca se
	// enteraba. Ahora: primero la clase nativa, y si no, se busca por reflexion
	// un bool llamado "Aiming" en el pawn, que es como lo llama el pack.
	bool bApuntando = false;
	if (const AShooterCharacter* Shooter = Cast<AShooterCharacter>(Char))
	{
		bApuntando = Shooter->IsAimingDownSights();
	}
	else if (Char)
	{
		static const FName NombreAiming(TEXT("Aiming"));
		if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Char->GetClass(), NombreAiming))
		{
			bApuntando = Prop->GetPropertyValue_InContainer(Char);
		}
	}
	const float ADSAlpha = bApuntando ? 1.0f : 0.0f;
	const float MotionScale = FMath::Lerp(1.0f, Bodycam.ADSMotionScale, ADSAlpha);

	FRotator ViewRot = OutVT.POV.Rotation;

	if (!bInitialized)
	{
		PrevViewRotation = ViewRot;
		PrevVelocity = Velocity;
		CurrentFOV = Bodycam.BaseFOV;
		bInitialized = true;
	}

	// ------------------------------------------------- movimiento corporal ----
	FVector LocalOffset(Bodycam.ForwardOffset, Bodycam.HorizontalOffset, Bodycam.VerticalOffset);
	float RollOffset = 0.0f;
	float PitchOffset = 0.0f;

	if (Bodycam.bMovementEnabled)
	{
		// --- bob de caminar/correr: la fase avanza con la DISTANCIA recorrida,
		//     asi el balanceo nace del movimiento real y no de un temporizador.
		const float BobSpeed = FMath::Lerp(Bodycam.WalkBobSpeed, Bodycam.SprintBobSpeed, SprintAlpha);
		const float BobAmount = FMath::Lerp(Bodycam.WalkBobAmount, Bodycam.SprintBobAmount, SprintAlpha)
			* SpeedAlpha * MotionScale * (bGrounded ? 1.0f : 0.25f);

		BobPhase += Speed2D * DeltaTime * BobSpeed;

		// armonico secundario -> no es una sinusoide perfecta (mas organico)
		const float S1 = FMath::Sin(BobPhase * 2.0f);
		const float S2 = FMath::Sin(BobPhase * 3.1f + 0.7f);
		const float L1 = FMath::Sin(BobPhase);

		LocalOffset.Z += (S1 * 0.85f + S2 * 0.15f) * BobAmount;
		LocalOffset.Y += L1 * BobAmount * 0.55f;
		RollOffset    += L1 * BobAmount * 0.18f;
		PitchOffset   += S1 * BobAmount * 0.06f;

		// Al correr al limite el torso baja un poco. Es una respuesta fisica
		// contenida, no un zoom ni un cambio de trayectoria de la bala.
		LocalOffset.Z -= SprintAlpha * SpeedAlpha * Bodycam.SprintCameraDrop * MotionScale;

		// --- inclinacion al desplazarse lateralmente
		const FVector Right = ViewRot.RotateVector(FVector::RightVector);
		const float StrafeDot = FVector::DotProduct(Velocity.GetSafeNormal2D(), Right) * SpeedAlpha;
		const float TargetRoll = -StrafeDot * Bodycam.StrafeTilt * MotionScale;
		CurrentRoll = FMath::FInterpTo(CurrentRoll, TargetRoll, DeltaTime, Bodycam.StrafeTiltSpeed);
		RollOffset += CurrentRoll;

		// --- retardo posicional por aceleracion (la camara "se queda atras")
		const FVector Accel = (Velocity - PrevVelocity) / DeltaTime;
		const FVector LocalAccel = ViewRot.UnrotateVector(Accel);
		const FVector TargetAccelOffset = (-LocalAccel * 0.0015f * Bodycam.AccelerationLag * MotionScale)
			.BoundToCube(4.0f);
		AccelOffset = FMath::VInterpTo(AccelOffset, TargetAccelOffset, DeltaTime, Bodycam.PositionLag);
		LocalOffset += AccelOffset;
	}

	// ------------------------------------------------------- respiracion ----
	if (Bodycam.bBreathingEnabled)
	{
		const float BreathRate = Bodycam.BreathingSpeed * (1.0f + Exertion * Bodycam.ExertionBreathSpeed);
		BreathPhase += DeltaTime * BreathRate;

		// Tres frecuencias ligeramente desfasadas: inspiracion/exhalacion deja de
		// parecer una onda perfecta sin introducir ruido o vibracion de mira.
		const float BreathWave =
			FMath::Sin(BreathPhase) * 0.72f +
			FMath::Sin(BreathPhase * 2.03f + 0.35f) * 0.18f +
			FMath::Sin(BreathPhase * 0.47f - 0.80f) * 0.10f;
		const float BreathSide = FMath::Sin(BreathPhase * 0.53f + 1.1f);
		const float MovementMask = FMath::Lerp(1.0f, Bodycam.BreathingMovingScale, SpeedAlpha);
		const float BreathAmount = Bodycam.BreathingAmount
			* (1.0f + Exertion * Bodycam.ExertionBreathAmount)
			* MovementMask * MotionScale;

		LocalOffset.Z += BreathWave * BreathAmount;
		LocalOffset.Y += BreathSide * BreathAmount * 0.10f;
		PitchOffset   += BreathWave * BreathAmount * 0.23f;
	}

	// ------------------------------------------------ microbalanceo humano ----
	if (Bodycam.bMicroSwayEnabled)
	{
		MicroSwayPhase += DeltaTime * Bodycam.MicroSwaySpeed * (1.0f + Exertion * 0.25f);
		const float Stillness = FMath::Lerp(1.0f, 0.28f, SpeedAlpha) * MotionScale;
		const float YawWave = FMath::Sin(MicroSwayPhase * 0.73f + 0.4f);
		const float PitchWave = FMath::Sin(MicroSwayPhase * 0.91f - 1.2f);
		const float RollWave = FMath::Sin(MicroSwayPhase * 0.51f + 2.0f);

		OutVT.POV.Rotation.Yaw   += YawWave * Bodycam.MicroSwayYaw * Stillness;
		PitchOffset               += PitchWave * Bodycam.MicroSwayPitch * Stillness;
		RollOffset                += RollWave * Bodycam.MicroSwayRoll * Stillness;
	}

	// --------------------------------------------------- salto / aterrizaje ----
	if (Bodycam.bLandingEnabled)
	{
		if (bWasGrounded && !bGrounded)
		{
			VerticalImpulse += Bodycam.JumpImpulse * MotionScale;
		}
		else if (!bWasGrounded && bGrounded)
		{
			const float FallSpeed = FMath::Abs(FMath::Min(PrevVelocity.Z, 0.0f));
			const float Strength = FMath::Min(Bodycam.LandingImpulse * (FallSpeed / 600.0f), Bodycam.LandingMaxImpulse);
			VerticalImpulse -= Strength * MotionScale;
		}
	}
	VerticalImpulse = FMath::FInterpTo(VerticalImpulse, 0.0f, DeltaTime, Bodycam.ImpulseRecoverySpeed);
	LocalOffset.Z += VerticalImpulse;
	PitchOffset += VerticalImpulse * 0.15f;

	// ------------------------------------------------ inercia al girar ----
	if (Bodycam.bMovementEnabled)
	{
		const FRotator DeltaRot = (ViewRot - PrevViewRotation).GetNormalized();
		TurnLag.Yaw   += DeltaRot.Yaw   * Bodycam.TurnInertia * MotionScale;
		TurnLag.Pitch += DeltaRot.Pitch * Bodycam.TurnInertia * MotionScale;
		TurnLag.Yaw   = FMath::Clamp(TurnLag.Yaw,   -Bodycam.MaxTurnLagDegrees, Bodycam.MaxTurnLagDegrees);
		TurnLag.Pitch = FMath::Clamp(TurnLag.Pitch, -Bodycam.MaxTurnLagDegrees, Bodycam.MaxTurnLagDegrees);
		TurnLag = FMath::RInterpTo(TurnLag, FRotator::ZeroRotator, DeltaTime, Bodycam.RotationLag);

		// la camara se retrasa una fraccion de segundo respecto al control
		OutVT.POV.Rotation.Yaw   -= TurnLag.Yaw;
		OutVT.POV.Rotation.Pitch -= TurnLag.Pitch;
		RollOffset -= TurnLag.Yaw * Bodycam.TurnRollCoupling;
	}

	// ------------------------------------------------------------- lean ----
	//  Asomarse por una esquina:  Q = izquierda,  E = derecha.
	//
	//  POR QUE SE LEE EL TECLADO A PELO: crear una accion de Enhanced Input
	//  obligaria a tocar el IMC del pack y su BP_Player. Preguntando la tecla
	//  al PlayerController no se toca nada suyo y no puede chocar.
	//
	//  QUE SE MUEVE: el desplazamiento lateral se aplica al COMPONENTE de
	//  camara, porque los brazos y el arma cuelgan de el; asi se asoman junto
	//  con la vista y el arma sigue perfectamente agarrada y centrada. La
	//  capsula, el torso y las piernas NO se tocan: el cuerpo se queda detras
	//  de la cobertura, que es justo lo que se pide.
	//
	//  LA INCLINACION va en la POV y no en el componente porque la camara usa
	//  bUsePawnControlRotation, o sea que ignora la rotacion de su padre.
	{
		float LeanDeseado = 0.0f;
		if (PCOwner)
		{
			if (PCOwner->IsInputKeyDown(EKeys::E)) { LeanDeseado += 1.0f; }
			if (PCOwner->IsInputKeyDown(EKeys::Q)) { LeanDeseado -= 1.0f; }
		}

		// PARED. Se traza hacia el lado desde la altura de los ojos. Si hay un
		// muro, el asomo se recorta a lo que cabe, de modo que la camara nunca
		// llega a atravesarlo.
		if (!FMath::IsNearlyZero(LeanDeseado) && Char && GetWorld())
		{
			const FVector Origen = OutVT.POV.Location;
			const FVector Lado = FRotationMatrix(
				FRotator(0.0f, OutVT.POV.Rotation.Yaw, 0.0f)).GetUnitAxis(EAxis::Y);
			const float Alcance = Bodycam.LeanSideOffset + GLeanMargenPared;

			FHitResult Golpe;
			FCollisionQueryParams Params(TEXT("BodycamLean"), false, Char);
			if (GetWorld()->LineTraceSingleByChannel(Golpe, Origen,
					Origen + Lado * Alcance * LeanDeseado, ECC_Visibility, Params))
			{
				const float Libre = FMath::Max(Golpe.Distance - GLeanMargenPared, 0.0f);
				LeanDeseado = FMath::Sign(LeanDeseado)
					* FMath::Min(FMath::Abs(LeanDeseado), Libre / Alcance);
			}
		}

		GLeanActual = FMath::FInterpTo(GLeanActual, LeanDeseado, DeltaTime, GLeanVelocidad);

		if (!FMath::IsNearlyZero(GLeanActual, 0.0005f) || !GLeanOffsetPrevio.IsNearlyZero())
		{
			UCameraComponent* CamLean = nullptr;
			if (AMyProject7Character* FPChar = Cast<AMyProject7Character>(Char))
			{
				CamLean = FPChar->GetFirstPersonCameraComponent();
			}
			if (!CamLean && Char)
			{
				CamLean = Char->FindComponentByClass<UCameraComponent>();
			}

			if (CamLean)
			{
				// Y = lateral, Z = se agacha un poco al asomarse (mas tactico).
				const FVector Nuevo(
					0.0f,
					Bodycam.LeanSideOffset * GLeanActual,
					-Bodycam.LeanDownOffset * FMath::Abs(GLeanActual));

				// Se DESHACE lo puesto el frame anterior antes de aplicar lo
				// nuevo: nunca se acumula, y se respeta lo que el pack le haga
				// a la camara por su cuenta (agacharse, etc.).
				CamLean->SetRelativeLocation(
					CamLean->GetRelativeLocation() - GLeanOffsetPrevio + Nuevo);
				GLeanOffsetPrevio = Nuevo;
			}

			RollOffset += Bodycam.LeanRollDegrees * GLeanActual;
		}
	}

	// ---------------------------------------------------------- judder ----
	// Una bodycam pierde fotogramas: la imagen da un tiron seco y vuelve. Es
	// sutil pero es de lo que mas la delata como camara y no como videojuego.
	if (Bodycam.JudderAmount > 0.0f && Bodycam.JudderPerSecond > 0.0f)
	{
		if (FMath::FRand() < Bodycam.JudderPerSecond * DeltaTime)
		{
			JudderOffset.Yaw   = FMath::FRandRange(-1.0f, 1.0f) * Bodycam.JudderAmount;
			JudderOffset.Pitch = FMath::FRandRange(-1.0f, 1.0f) * Bodycam.JudderAmount * 0.7f;
		}
		JudderOffset = FMath::RInterpTo(JudderOffset, FRotator::ZeroRotator, DeltaTime, 22.0f);
		OutVT.POV.Rotation.Yaw   += JudderOffset.Yaw;
		OutVT.POV.Rotation.Pitch += JudderOffset.Pitch;
	}

	// Shake corto, fisico y aditivo. Es solo POV: no altera la trayectoria,
	// precision, sockets ni la rotacion real del personaje.
	const float ShotScale = FMath::Lerp(1.0f, Bodycam.ShotADSMotionScale, ADSAlpha);
	OutVT.POV.Rotation.Pitch += ShotImpulse.Pitch * ShotScale;
	OutVT.POV.Rotation.Yaw   += ShotImpulse.Yaw * ShotScale;
	OutVT.POV.Rotation.Roll  += ShotImpulse.Roll * ShotScale;
	ShotImpulse = FMath::RInterpTo(ShotImpulse, FRotator::ZeroRotator,
		DeltaTime, Bodycam.ShotRecoverySpeed);

	// --------------------------------------------------------- aplicar ----
	OutVT.POV.Location += ViewRot.RotateVector(LocalOffset);
	OutVT.POV.Rotation.Roll  += RollOffset;
	OutVT.POV.Rotation.Pitch += PitchOffset;

	// ------------------------------------------------------------- optica ----
	if (Bodycam.bLensEnabled)
	{
		const float LensScale = FMath::Lerp(1.0f, Bodycam.ADSLensScale, ADSAlpha);
		// gran angular: el FOV base ya es amplio; la "distorsion" lo ensancha un poco mas
		const float WideFOV = Bodycam.BaseFOV + Bodycam.LensDistortionStrength * 8.0f * LensScale;
		const float TargetFOV = FMath::Lerp(WideFOV, Bodycam.ADSFOV, ADSAlpha);
		CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, Bodycam.FOVInterpSpeed);
		OutVT.POV.FOV = CurrentFOV;

		// El arma/brazos se renderizan con su propio FirstPersonFieldOfView, que el
		// template afina EN PAREJA con FirstPersonScale. Solo lo tocamos si el
		// usuario lo pide explicitamente (ratio > 0); por defecto se respeta.
		// El arma y las manos se dibujan con SU PROPIO FOV. Con un ratio MENOR
		// que 1 se ven mas grandes y sin la deformacion de gran angular, que es
		// justo lo que se quiere: lente ancha en el mundo, arma limpia delante.
		//
		// Antes solo funcionaba con AMyProject7Character. Se busca la camara de
		// forma generica para que valga con cualquier pawn, incluido el
		// BP_Player del pack.
		if (Bodycam.FirstPersonFOVRatio > 0.0f && Char)
		{
			UCameraComponent* CamFP = nullptr;
			if (AMyProject7Character* FPChar = Cast<AMyProject7Character>(Char))
			{
				CamFP = FPChar->GetFirstPersonCameraComponent();
			}
			if (!CamFP)
			{
				CamFP = Char->FindComponentByClass<UCameraComponent>();
			}
			if (CamFP)
			{
				CamFP->FirstPersonFieldOfView = CurrentFOV * Bodycam.FirstPersonFOVRatio;
				CamFP->bEnableFirstPersonFieldOfView = true;
			}
		}
	}

	// -------------------------------------------------------- postproceso ----
	if (Bodycam.bPostProcessEnabled)
	{
		const float LensScale = FMath::Lerp(1.0f, Bodycam.ADSLensScale, ADSAlpha);
		FPostProcessSettings& PP = OutVT.POV.PostProcessSettings;

		// Vineta nativa muy ligera: solo define las esquinas, sin cerrar la imagen.
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = FMath::Clamp(Bodycam.Vignette * 0.55f, 0.0f, 1.0f);

		// Aberracion cromatica: SOLO en la periferia. StartOffset alto = el centro
		// (mira, arma, enemigos de frente) queda limpio y sin franjas de color.
		// OJO: SceneFringeIntensity util va de 0 a ~1.5; valores de 5 pintaban
		// arcoiris en toda la pantalla.
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = FMath::Clamp(
			Bodycam.ChromaticAberration * (1.0f + Bodycam.LensDistortionStrength * 0.5f) * LensScale,
			0.0f, 1.5f);
		PP.bOverride_ChromaticAberrationStartOffset = true;
		PP.ChromaticAberrationStartOffset = Bodycam.ChromaticAberrationStartOffset;

		PP.bOverride_MotionBlurAmount = true;
		PP.MotionBlurAmount = Bodycam.MotionBlur;
		PP.bOverride_BloomDirtMask = true;
		PP.BloomDirtMask = LensDirtMask;
		PP.bOverride_BloomDirtMaskIntensity = true;
		PP.BloomDirtMaskIntensity = LensDirtMask ? Bodycam.LensDirtIntensity : 0.0f;
		PP.bOverride_BloomDirtMaskTint = true;
		PP.BloomDirtMaskTint = FLinearColor::White;
		// El fogonazo y el laser son emisivos. Un bloom alto los expandia por
		// media pantalla y hacia parecer cromada incluso un arma mate.
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = Bodycam.BloomIntensity;

		// grano de sensor + desaturacion de camara barata
		PP.bOverride_FilmGrainIntensity = true;
		PP.FilmGrainIntensity = Bodycam.FilmGrain;
		PP.bOverride_ColorSaturation = true;
		PP.ColorSaturation = FVector4(Bodycam.Saturation, Bodycam.Saturation, Bodycam.Saturation, 1.0f);
		// Ajustable al instante desde la consola del juego, sin volver a abrir
		// el editor ni tocar el resto de ajustes de la bodycam.
		{
			const FString Color = CVarBodycamColor.GetValueOnGameThread();
			if (!Color.IsEmpty())
			{
				TArray<FString> P;
				Color.ParseIntoArrayWS(P);
				if (P.Num() >= 1) { Bodycam.Saturation = FMath::Clamp(FCString::Atof(*P[0]), 0.0f, 2.0f); }
				if (P.Num() >= 2) { Bodycam.Contrast = FMath::Clamp(FCString::Atof(*P[1]), 0.5f, 2.0f); }
				PP.ColorSaturation = FVector4(Bodycam.Saturation, Bodycam.Saturation, Bodycam.Saturation, 1.0f);
			}
		}

		// Contraste global del grading, aparte del contraste local de sombras
		// que ya se ajusta mas abajo.
		PP.bOverride_ColorContrast = true;
		PP.ColorContrast = FVector4(Bodycam.Contrast, Bodycam.Contrast, Bodycam.Contrast, 1.0f);

		// Distorsion barrel optimizada en un unico material post-process. La
		// mascara empieza fuera del centro para no deformar mira, arma ni objetivo.
		if (!BodycamLensMID && BodycamLensMaterial)
		{
			BodycamLensMID = UMaterialInstanceDynamic::Create(BodycamLensMaterial, this);
		}
		if (BodycamLensMID)
		{
			{
				const FString L = CVarBodycamLens.GetValueOnGameThread();
				if (!L.IsEmpty())
				{
					TArray<FString> P;
					L.ParseIntoArrayWS(P);
					if (P.Num() >= 1) { Bodycam.LensDistortionStrength = FCString::Atof(*P[0]); }
					if (P.Num() >= 2) { Bodycam.LensEdgeStart = FCString::Atof(*P[1]); }
					if (P.Num() >= 3) { Bodycam.LensEdgeEnd   = FCString::Atof(*P[2]); }
					if (P.Num() >= 4) { Bodycam.Vignette       = FCString::Atof(*P[3]); }
				}
			}
			BodycamLensMID->SetScalarParameterValue(TEXT("LensDistortion"),
				Bodycam.LensDistortionStrength * LensScale);
			BodycamLensMID->SetScalarParameterValue(TEXT("EdgeStart"), Bodycam.LensEdgeStart);
			BodycamLensMID->SetScalarParameterValue(TEXT("EdgeEnd"), Bodycam.LensEdgeEnd);
			BodycamLensMID->SetScalarParameterValue(TEXT("VignetteIntensity"), Bodycam.Vignette);
			PP.AddBlendable(BodycamLensMID, 1.0f);
		}

		// --- EXPOSICION LENTA: la firma de una camara corporal ---------------
		// Al salir de un tunel a la calle la imagen se queda quemada un momento
		// y luego cierra el diafragma; al entrar en sombra pasa lo contrario.
		// Un juego normal expone al instante; una bodycam, no.
		// No heredar el modo manual/fisico de una camara o asset de escenario.
		// En mapas de Fab eso puede dejar la imagen completamente negra en Game,
		// aunque el viewport del editor se vea correctamente expuesto.
		PP.bOverride_AutoExposureMethod = true;
		PP.AutoExposureMethod = AEM_Histogram;
		PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		PP.AutoExposureApplyPhysicalCameraExposure = false;

		PP.bOverride_AutoExposureSpeedUp = true;
		PP.AutoExposureSpeedUp = Bodycam.ExposureAdaptSpeed;
		PP.bOverride_AutoExposureSpeedDown = true;
		PP.AutoExposureSpeedDown = Bodycam.ExposureAdaptSpeed * 0.8f;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = Bodycam.ExposureBias;

		// Lectura de la cval de calibracion en caliente
		{
			const FString L = CVarBodycamExposure.GetValueOnGameThread();
			if (!L.IsEmpty())
			{
				TArray<FString> P;
				L.ParseIntoArrayWS(P);
				if (P.Num() >= 1) { Bodycam.ExposureBias  = FCString::Atof(*P[0]); }
				if (P.Num() >= 2) { Bodycam.ExposureMinEV = FCString::Atof(*P[1]); }
				if (P.Num() >= 3) { Bodycam.ExposureMaxEV = FCString::Atof(*P[2]); }
				PP.AutoExposureBias = Bodycam.ExposureBias;
			}
		}

		// Rango del diafragma. Si no se fija, manda el volumen de post-proceso
		// del mapa y un interior puede quedarse a oscuras.
		// Se ajusta en vivo con:  bodycam.exposure <bias> <min> <max>
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.AutoExposureMinBrightness = Bodycam.ExposureMinEV;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMaxBrightness = FMath::Max(Bodycam.ExposureMaxEV, Bodycam.ExposureMinEV + 0.1f);

		// Recupera detalle de estanterias y paredes oscuras sin convertir la
		// imagen completa en gris ni quemar las luminarias del almacen.
		PP.bOverride_LocalExposureShadowContrastScale = true;
		PP.LocalExposureShadowContrastScale = 0.70f;
		PP.bOverride_LocalExposureHighlightContrastScale = true;
		PP.LocalExposureHighlightContrastScale = 0.85f;

		OutVT.POV.PostProcessBlendWeight = 1.0f;
	}

	// ------------------------------------------------------------ memoria ----
	PrevViewRotation = ViewRot;
	PrevVelocity = Velocity;
	bWasGrounded = bGrounded;
}
