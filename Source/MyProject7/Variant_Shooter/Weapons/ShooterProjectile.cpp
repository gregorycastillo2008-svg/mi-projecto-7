// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterProjectile.h"
#include "Components/MeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

AShooterProjectile::AShooterProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// create the collision component and assign it as the root
	RootComponent = CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));

	CollisionComponent->SetSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// create the projectile movement component. No need to attach it because it's not a Scene Component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));

	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bShouldBounce = true;

	// set the default damage type
	HitDamageType = UDamageType::StaticClass();
}

void AShooterProjectile::BeginPlay()
{
	Super::BeginPlay();

	// BALA REAL: el template dispara un dardo de gomaespuma visible. Una bala
	// de verdad no se ve en vuelo, asi que se ocultan las mallas del proyectil.
	// El impacto y el sonido siguen funcionando igual.
	TArray<UMeshComponent*> Mallas;
	GetComponents<UMeshComponent>(Mallas);
	for (UMeshComponent* M : Mallas)
	{
		M->SetVisibility(false, true);
		M->SetHiddenInGame(true, true);
	}
	
	// ignore the pawn that shot this projectile
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
}

void AShooterProjectile::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the destruction timer
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
}

void AShooterProjectile::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// ignore if we've already hit something else
	if (bHit)
	{
		return;
	}

	bHit = true;

	// disable collision on the projectile
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// IMPACTO FISICO: la bala ya llega desde la boca del arma; al chocar deja
	// una marca corta y usa el efecto apropiado para el material. Es puramente
	// visual, por eso no altera daño, colisiones ni la dirección del proyectil.
	if (OtherComp && !Cast<ACharacter>(Other))
	{
		static UMaterialInterface* AgujeroBala = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Scene_Warehouse/Environment/Decals/M_Decal_PersistentBulletHole.M_Decal_PersistentBulletHole"));
		static UMaterialInterface* AgujeroBalaRespaldo = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/BODYCAM_VFX/M_BulletHole.M_BulletHole"));
		static UParticleSystem* ChispaMetal = LoadObject<UParticleSystem>(nullptr,
			TEXT("/Game/MilitaryWeapDark/FX/P_Impact_Metal_Small_01.P_Impact_Metal_Small_01"));
		static UParticleSystem* PolvoPiedra = LoadObject<UParticleSystem>(nullptr,
			TEXT("/Game/MilitaryWeapDark/FX/P_Impact_Stone_Small_01.P_Impact_Stone_Small_01"));
		static UParticleSystem* AstillasMadera = LoadObject<UParticleSystem>(nullptr,
			TEXT("/Game/MilitaryWeapDark/FX/P_Impact_Wood_Small_01.P_Impact_Wood_Small_01"));

		const UMaterialInterface* Material = OtherComp->GetMaterial(0);
		const FString NombreMaterial = Material ? Material->GetName().ToLower() : FString();
		const bool bMetal = NombreMaterial.Contains(TEXT("metal")) || NombreMaterial.Contains(TEXT("steel"))
			|| NombreMaterial.Contains(TEXT("iron")) || NombreMaterial.Contains(TEXT("rack"))
			|| NombreMaterial.Contains(TEXT("barrel"));
		const bool bMadera = NombreMaterial.Contains(TEXT("wood")) || NombreMaterial.Contains(TEXT("timber"));
		UParticleSystem* EfectoImpacto = bMetal ? ChispaMetal : (bMadera ? AstillasMadera : PolvoPiedra);

		if (EfectoImpacto)
		{
			const float Escala = FMath::FRandRange(0.72f, 1.05f);
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EfectoImpacto,
				Hit.ImpactPoint + Hit.ImpactNormal * 1.5f, Hit.ImpactNormal.Rotation(),
				FVector(Escala), true);
		}

		if (UMaterialInterface* DecalMaterial = AgujeroBala ? AgujeroBala : AgujeroBalaRespaldo)
		{
			// Vida limitada para conservar rendimiento en ráfagas y escopeta.
			UGameplayStatics::SpawnDecalAttached(DecalMaterial, FVector(7.0f, 5.0f, 5.0f),
				OtherComp, NAME_None, Hit.ImpactPoint + Hit.ImpactNormal * 0.35f,
				Hit.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition, 0.0f);
		}
	}

	// make AI perception noise
	MakeNoise(NoiseLoudness, GetInstigator(), GetActorLocation(), NoiseRange, NoiseTag);

	if (bExplodeOnHit)
	{
		
		// apply explosion damage centered on the projectile
		ExplosionCheck(GetActorLocation());

	} else {

		// single hit projectile. Process the collided actor
		ProcessHit(Other, OtherComp, Hit.ImpactPoint, -Hit.ImpactNormal);

	}

	// pass control to BP for any extra effects
	BP_OnProjectileHit(Hit);

	// check if we should schedule deferred destruction of the projectile
	if (DeferredDestructionTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &AShooterProjectile::OnDeferredDestruction, DeferredDestructionTime, false);

	} else {

		// destroy the projectile right away
		Destroy();
	}
}

void AShooterProjectile::ExplosionCheck(const FVector& ExplosionCenter)
{
	// do a sphere overlap check look for nearby actors to damage
	TArray<FOverlapResult> Overlaps;

	FCollisionShape OverlapShape;
	OverlapShape.SetSphere(ExplosionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (!bDamageOwner)
	{
		QueryParams.AddIgnoredActor(GetInstigator());
	}

	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionCenter, FQuat::Identity, ObjectParams, OverlapShape, QueryParams);

	TArray<AActor*> DamagedActors;

	// process the overlap results
	for (const FOverlapResult& CurrentOverlap : Overlaps)
	{
		// overlaps may return the same actor multiple times per each component overlapped
		// ensure we only damage each actor once by adding it to a damaged list
		if (DamagedActors.Find(CurrentOverlap.GetActor()) == INDEX_NONE)
		{
			DamagedActors.Add(CurrentOverlap.GetActor());

			// apply physics force away from the explosion
			const FVector& ExplosionDir = CurrentOverlap.GetActor()->GetActorLocation() - GetActorLocation();

			// push and/or damage the overlapped actor
			ProcessHit(CurrentOverlap.GetActor(), CurrentOverlap.GetComponent(), GetActorLocation(), ExplosionDir.GetSafeNormal());
		}
			
	}
}

void AShooterProjectile::ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection)
{
	// have we hit a character?
	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		// ignore the owner of this projectile
		if (HitCharacter != GetOwner() || bDamageOwner)
		{
			// apply damage to the character
			UGameplayStatics::ApplyDamage(HitCharacter, HitDamage, GetInstigator()->GetController(), this, HitDamageType);
		}
	}

	// have we hit a physics object?
	if (HitComp->IsSimulatingPhysics())
	{
		// give some physics impulse to the object
		HitComp->AddImpulseAtLocation(HitDirection * PhysicsForce, HitLocation);
	}
}

void AShooterProjectile::OnDeferredDestruction()
{
	// destroy this actor
	Destroy();
}

void AShooterProjectile::SetNoiseTag(const FName& Tag)
{
	NoiseTag = Tag;
}
