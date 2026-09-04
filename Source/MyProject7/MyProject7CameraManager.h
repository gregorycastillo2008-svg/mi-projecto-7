// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MyProject7CameraManager.generated.h"

/**
 *  Ajustes de la camara BODYCAM. Todo configurable desde el BP del CameraManager.
 *  Los efectos se aplican SOLO al POV final (UpdateViewTarget) -> no tocan el
 *  personaje, los sockets, las manos, el arma ni la trayectoria de la bala.
 */
USTRUCT(BlueprintType)
struct FBodycamCameraSettings
{
	GENERATED_BODY()

	// ---- Interruptores de debug -------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bodycam|Debug") bool bBodycamEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Debug") bool bMovementEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Debug") bool bLensEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Debug") bool bPostProcessEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Debug") bool bLandingEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Debug") bool bBreathingEnabled = true;

	// ---- Posicion de la camara respecto a los ojos (espacio camara, cm) ----
	/** OJO: la camara cuelga del socket "head" de los brazos, asi que moverla hacia
	 *  DELANTE la acerca al arma y la mete dentro. Para sensacion bodycam mejor
	 *  bajarla un poco y, si acaso, retrasarla. Arranca neutro: dialo a tu gusto. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Position", meta = (ClampMin = -30, ClampMax = 30))
	// Negativo = la camara retrocede, asi entra mas arma en cuadro.
	float ForwardOffset = -11.0f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Position", meta = (ClampMin = -30, ClampMax = 30))
	float VerticalOffset = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Position", meta = (ClampMin = -30, ClampMax = 30))
	float HorizontalOffset = 0.0f;

	// ---- Optica -----------------------------------------------------------
	/** 100 = gran angular de bodycam pero con el CENTRO recto. Por encima de ~108
	 *  el centro empieza a estirarse y el arma se deforma. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lens", meta = (ClampMin = 60, ClampMax = 140))
	float BaseFOV = 107.0f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lens", meta = (ClampMin = 30, ClampMax = 110))
	float ADSFOV = 70.0f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lens", meta = (ClampMin = 1, ClampMax = 30))
	float FOVInterpSpeed = 12.0f;
	/** Barrel/gran angular: ensancha el FOV y sube la aberracion en bordes. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lens", meta = (ClampMin = 0, ClampMax = 1))
	// A CERO: sin curvatura de lente. El borde negro se mantiene igual, lo
	// controlan LensEdgeStart/End y Vignette, que son independientes.
	float LensDistortionStrength = 0.13f;
	/** Radio normalizado a partir del cual empieza a curvarse la periferia. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lens", meta = (ClampMin = 0.05, ClampMax = 0.49))
	// OJO: ahora se compara contra r (0 a 1.02 en 16:9), no contra r*r (0 a 0.5).
	// r=0.5 es el centro del borde superior; r=0.89 el lateral; r=1.02 la esquina.
	float LensEdgeStart = 0.55f;         // solo la periferia se curva y oscurece

	/** Donde el borde llega a negro del todo */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lens", meta = (ClampMin = 0.3, ClampMax = 1.5))
	float LensEdgeEnd = 1.02f;           // negro contenido en las esquinas
	/** El arma/brazos usan su PROPIO FOV (FirstPersonFieldOfView), afinado por el
	 *  template junto con FirstPersonScale. Dejar en 0 = NO tocarlo (recomendado:
	 *  el gran angular se aplica solo al mundo y el arma conserva su tamaño). */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lens", meta = (ClampMin = 0, ClampMax = 1.2))
	float FirstPersonFOVRatio = 0.48f;   // mas bajo = arma mas cerca / mas grande

	// ---- Balanceo al andar / correr ---------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 10))
	float WalkBobAmount = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 0.2))
	float WalkBobSpeed = 0.018f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 20))
	float SprintBobAmount = 1.05f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 0.2))
	float SprintBobSpeed = 0.023f;
	/** Ligera bajada del torso a maxima carrera; no altera la punteria ni el arma. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 5))
	float SprintCameraDrop = 0.85f;

	// ---- Respiracion en reposo --------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 3))
	float BreathingAmount = 0.10f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 5))
	float BreathingSpeed = 1.3f;
	/** La respiracion nunca desaparece por completo al moverse; simplemente queda
	 *  tapada por los pasos. Evita el corte artificial al arrancar/caminar. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 1))
	float BreathingMovingScale = 0.32f;
	/** Cuanto aumenta la amplitud respiratoria despues de correr. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 3))
	float ExertionBreathAmount = 0.35f;
	/** Cuanto se acelera la respiracion despues de correr. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 3))
	float ExertionBreathSpeed = 0.70f;
	/** Rapidez con la que se acumula y se recupera el cansancio visual. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0.05, ClampMax = 8))
	float ExertionBuildSpeed = 2.4f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0.02, ClampMax = 3))
	float ExertionRecoverySpeed = 0.22f;

	// ---- Microbalanceo humano ---------------------------------------------
	/** Movimiento casi imperceptible de torso/cuello. En ADS queda reducido por
	 *  ADSMotionScale, por lo que aporta vida sin sacar la mira del objetivo. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|MicroMotion") bool bMicroSwayEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Bodycam|MicroMotion", meta = (ClampMin = 0, ClampMax = 1))
	float MicroSwayYaw = 0.035f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|MicroMotion", meta = (ClampMin = 0, ClampMax = 1))
	float MicroSwayPitch = 0.025f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|MicroMotion", meta = (ClampMin = 0, ClampMax = 1))
	float MicroSwayRoll = 0.018f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|MicroMotion", meta = (ClampMin = 0.05, ClampMax = 4))
	float MicroSwaySpeed = 0.72f;
	/** Inclinacion secundaria del torso al iniciar un giro brusco. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|MicroMotion", meta = (ClampMin = 0, ClampMax = 1))
	float TurnRollCoupling = 0.22f;

	// ---- Inercia -----------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bodycam|Inertia", meta = (ClampMin = 1, ClampMax = 30))
	float RotationLag = 10.5f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Inertia", meta = (ClampMin = 0, ClampMax = 1))
	float TurnInertia = 0.14f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Inertia", meta = (ClampMin = 0, ClampMax = 8))
	float MaxTurnLagDegrees = 1.10f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Inertia", meta = (ClampMin = 1, ClampMax = 30))
	float PositionLag = 8.5f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Inertia", meta = (ClampMin = 0, ClampMax = 5))
	float AccelerationLag = 0.75f;

	// ---- Inclinacion lateral ----------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 0, ClampMax = 6))
	float StrafeTilt = 1.05f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Motion", meta = (ClampMin = 1, ClampMax = 20))
	float StrafeTiltSpeed = 6.0f;

	// ---- Salto / aterrizaje -------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bodycam|Impact", meta = (ClampMin = 0, ClampMax = 20))
	float JumpImpulse = 2.2f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Impact", meta = (ClampMin = 0, ClampMax = 40))
	float LandingImpulse = 9.0f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Impact", meta = (ClampMin = 0, ClampMax = 40))
	float LandingMaxImpulse = 14.0f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Impact", meta = (ClampMin = 1, ClampMax = 30))
	float ImpulseRecoverySpeed = 7.5f;

	// ---- Post proceso -------------------------------------------------------
	/** Viñeta: es radial de verdad, oscurece SOLO las esquinas. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 2))
	float Vignette = 0.05f;             // 5%: borde bodycam discreto, vision frontal limpia
	/** Aberracion cromatica en la periferia. Util 0-1.5; por encima pinta arcoiris. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 1.5))
	float ChromaticAberration = 0.10f;   // franja de color en la periferia
	/** Refuerzo de la viñeta en las esquinas (0 = nada). No toca el color global. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 1))
	float EdgeDarkening = 0.0f;
	/** Desde donde (0=centro, 1=borde) empieza la aberracion. Alto = centro LIMPIO
	 *  para apuntar y distorsion solo en los bordes -> look de lente bodycam. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 1))
	float ChromaticAberrationStartOffset = 0.72f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 1))
	float MotionBlur = 0.35f;
	/** Grano de sensor barato */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 2))
	float FilmGrain = 0.035f;
	/** Bloom de luces intensas, sin expandir emisivos por toda la pantalla. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 2))
	float BloomIntensity = 0.18f;
	/** Suciedad de sensor extremadamente tenue: solo se percibe sobre luces muy fuertes. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 1))
	float LensDirtIntensity = 0.012f;
	/** Saturacion contenida de una bodycam profesional. 1.15 = +15% de color. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0, ClampMax = 2))
	float Saturation = 1.3f;            // +30% de color: vivo sin sobresaturar

	/** Contraste global. 1 = sin tocar; por encima separa mas luces y sombras. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|PostProcess", meta = (ClampMin = 0.5, ClampMax = 2))
	float Contrast = 1.0f;   // +15% pedido

	// ---- Sensor: lo que delata una camara barata de verdad ------------------
	/** Velocidad de adaptacion de la exposicion. BAJA = al pasar de sombra a sol
	 *  la imagen se queda quemada un instante y luego cierra, como una bodycam. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Sensor", meta = (ClampMin = 0.05, ClampMax = 20))
	float ExposureAdaptSpeed = 1.35f;

	/** Margen de exposicion permitido (una bodycam tiene poco rango) */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Sensor", meta = (ClampMin = -8, ClampMax = 8))
	// Evita que ventanas y superficies claras se quemen: conserva detalle en
	// exteriores sin convertir el interior industrial en una imagen oscura.
	float ExposureBias = 0.15f;

	// El proyecto arranca con r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange
	// activado, asi que estos dos van en EV100, NO en luminancia cruda.
	// Sin fijarlos aqui mandaba el volumen de post-proceso del mapa, y por eso un
	// interior se veia oscuro en juego aunque el editor lo pintara claro: el
	// viewport del editor no ejecuta este PlayerCameraManager.
	/** Lo mas que puede ABRIR el diafragma (EV100). Mas negativo = ve mas en penumbra */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Sensor", meta = (ClampMin = -10, ClampMax = 5))
	float ExposureMinEV = -6.0f;

	/** Lo mas que puede CERRAR (EV100). Mas alto = no se quema al mirar a la luz */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Sensor", meta = (ClampMin = -5, ClampMax = 12))
	float ExposureMaxEV = 5.0f;

	/** Micro-tirones de imagen, como fotogramas perdidos. Grados de salto. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Sensor", meta = (ClampMin = 0, ClampMax = 3))
	float JudderAmount = 0.0f;

	/** Probabilidad por segundo de que se pierda un fotograma */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Sensor", meta = (ClampMin = 0, ClampMax = 10))
	float JudderPerSecond = 0.0f;

	// ---- Reaccion fisica al disparo -----------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bodycam|Shot", meta = (ClampMin = 0, ClampMax = 3))
	float ShotPitch = 0.38f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Shot", meta = (ClampMin = 0, ClampMax = 2))
	float ShotYaw = 0.12f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Shot", meta = (ClampMin = 0, ClampMax = 2))
	float ShotRoll = 0.18f;
	UPROPERTY(EditAnywhere, Category = "Bodycam|Shot", meta = (ClampMin = 1, ClampMax = 40))
	float ShotRecoverySpeed = 18.0f;
	/** Amortiguacion adicional del shake de disparo cuando se apunta. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Shot", meta = (ClampMin = 0, ClampMax = 1))
	float ShotADSMotionScale = 0.45f;

	// ---- Lean (asomarse por una esquina) --------------------------------------
	/** Grados de inclinacion de la camara al asomarse del todo. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lean", meta = (ClampMin = 0, ClampMax = 35))
	float LeanRollDegrees = 13.0f;
	/** Cuanto se desplaza la camara lateralmente al asomarse (cm). */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lean", meta = (ClampMin = 0, ClampMax = 90))
	float LeanSideOffset = 38.0f;
	/** Cuanto baja la camara al asomarse (cm) - el cuerpo se dobla. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|Lean", meta = (ClampMin = 0, ClampMax = 30))
	float LeanDownOffset = 7.0f;

	// ---- Reduccion al apuntar ------------------------------------------------
	/** Cuanto movimiento bodycam SOBREVIVE en ADS (0 = camara totalmente estable). */
	UPROPERTY(EditAnywhere, Category = "Bodycam|ADS", meta = (ClampMin = 0, ClampMax = 1))
	float ADSMotionScale = 0.15f;
	/** Cuanta distorsion de lente sobrevive en ADS. */
	UPROPERTY(EditAnywhere, Category = "Bodycam|ADS", meta = (ClampMin = 0, ClampMax = 1))
	float ADSLensScale = 0.70f;
};

/**
 *  Camera manager de primera persona con capa BODYCAM.
 *  Limita el pitch y aplica el efecto de camara corporal sobre el POV final.
 */
UCLASS()
class AMyProject7CameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	AMyProject7CameraManager();

	/** Ajustes de la camara bodycam */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bodycam")
	FBodycamCameraSettings Bodycam;

	/** Impulso visual sincronizado con el disparo. No cambia punteria ni control. */
	UFUNCTION(BlueprintCallable, Category = "Bodycam")
	void AddShotImpulse(float Strength = 1.0f);

protected:

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

private:

	/** Aplica la capa bodycam al POV ya calculado */
	void ApplyBodycam(FTViewTarget& OutVT, float DeltaTime);

	// ---- Estado procedural (persistente entre frames) ----------------------
	float BobPhase = 0.0f;
	float BreathPhase = 0.0f;
	float MicroSwayPhase = 0.0f;
	float Exertion = 0.0f;
	float SmoothedSpeedAlpha = 0.0f;
	float CurrentRoll = 0.0f;
	float VerticalImpulse = 0.0f;
	float CurrentFOV = 0.0f;
	FRotator TurnLag = FRotator::ZeroRotator;
	FRotator PrevViewRotation = FRotator::ZeroRotator;
	FVector  AccelOffset = FVector::ZeroVector;
	FVector  PrevVelocity = FVector::ZeroVector;
	FRotator JudderOffset = FRotator::ZeroRotator;
	FRotator ShotImpulse = FRotator::ZeroRotator;

	/** Material periferico: centro sin deformar, barrel/vineta solo en bordes. */
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInterface> BodycamLensMaterial = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> BodycamLensMID = nullptr;
	/** Mascara de suciedad para el bloom; no afecta mira, arma ni nitidez central. */
	UPROPERTY(Transient)
	TObjectPtr<class UTexture> LensDirtMask = nullptr;
	bool bWasGrounded = true;
	bool bInitialized = false;
};
