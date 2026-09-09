# Graph Report - MyProject7  (2026-09-09)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 671 nodes · 1013 edges · 52 communities (30 shown, 5 thin omitted)
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 16 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `17c9a954`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- FBodycamCameraSettings
- Automation_MyProject7.sln
- ShooterStateTreeUtility.cpp
- FStateTreeSenseEnemiesInstanceData
- ShooterPickup.cpp
- ShooterWeapon.cpp
- BattlefieldTrenchGenerator.cpp
- ShooterAIController.cpp
- ShooterCharacter.cpp
- AMyProject7CameraManager
- AMyProject7Character
- ShooterProjectile.cpp
- HorrorPlayerController.cpp
- ShooterNPC.cpp
- ShooterPlayerController.cpp
- ShooterGameMode.cpp
- Backup_RiderSetup_20260902_224828/MyProject7.sln
- MyProject7PlayerController.cpp
- HorrorCharacter.cpp
- UnrealBuildTool
- AShooterWeapon
- .GetFirstPersonCameraComponent
- MyProject7CameraManager.cpp
- build_project7_exterior_phase2_lighting.py
- .GetFirstPersonMesh
- ShooterCharacter.h
- AHorrorGameMode::ChoosePlayerStart_Implementation
- AMyProject7GameMode
- build_project7_exterior_phase1.py
- build_project7_exterior_phase3_detail.py
- ShooterNPCSpawner.h
- HorrorPlayerController.h
- AShooterCharacter::TakeDamage
- ShooterGameMode.h
- AShooterCharacter::SetTeam

## God Nodes (most connected - your core abstractions)
1. `FBodycamCameraSettings` - 73 edges
2. `AMyProject7CameraManager` - 34 edges
3. `AMyProject7Character` - 20 edges
4. `FStateTreeSenseEnemiesInstanceData` - 20 edges
5. `FStateTreeLineOfSightToTargetConditionInstanceData` - 10 edges
6. `AShooterWeapon()` - 10 edges
7. `FWeaponTableRow` - 9 edges
8. `ABattlefieldTrenchGenerator::BuildRoute()` - 7 edges
9. `SurfaceHeight()` - 7 edges
10. `UnrealBuildTool` - 7 edges

## Surprising Connections (you probably didn't know these)
- `AMyProject7PlayerController::AMyProject7PlayerController()` --references--> `AMyProject7CameraManager`  [EXTRACTED]
  Source/MyProject7/MyProject7PlayerController.cpp → Source/MyProject7/MyProject7CameraManager.h
- `AHorrorPlayerController::AHorrorPlayerController()` --references--> `AMyProject7CameraManager`  [EXTRACTED]
  Source/MyProject7/Variant_Horror/HorrorPlayerController.cpp → Source/MyProject7/MyProject7CameraManager.h
- `AShooterPlayerController::AShooterPlayerController()` --references--> `AMyProject7CameraManager`  [EXTRACTED]
  Source/MyProject7/Variant_Shooter/ShooterPlayerController.cpp → Source/MyProject7/MyProject7CameraManager.h
- `AShooterPlayerController::PostInitializeComponents()` --references--> `AMyProject7CameraManager`  [EXTRACTED]
  Source/MyProject7/Variant_Shooter/ShooterPlayerController.cpp → Source/MyProject7/MyProject7CameraManager.h
- `AMyProject7Character` --inherits--> `ACharacter`  [EXTRACTED]
  Source/MyProject7/MyProject7Character.h → Source/MyProject7/Variant_Shooter/Weapons/ShooterProjectile.h

## Import Cycles
- None detected.

## Communities (52 total, 5 thin omitted)

### Community 0 - "FBodycamCameraSettings"
Cohesion: 0.03
Nodes (72): FBodycamCameraSettings, AccelerationLag, ADSFOV, ADSLensScale, ADSMotionScale, BaseFOV, bBodycamEnabled, bBreathingEnabled (+64 more)

### Community 1 - "Automation_MyProject7.sln"
Cohesion: 0.13
Nodes (52): VisionOS.Automation, Android.Automation, Apple.Automation, AutomationTool, AutomationUtils.Automation, BuildGraph.Automation, CookedEditor.Automation, CrowdinLocalization.Automation (+44 more)

### Community 2 - "ShooterStateTreeUtility.cpp"
Cohesion: 0.10
Nodes (33): EStateTreeNodeFormatting, EStateTreeRunStatus, FGuid, FStateTreeConditionCommonBase, FStateTreeDataView, FStateTreeExecutionContext, FStateTreeTaskCommonBase, FStateTreeTransitionResult (+25 more)

### Community 3 - "FStateTreeSenseEnemiesInstanceData"
Cohesion: 0.06
Nodes (41): FStateTreeDelegateDispatcher, AAIController, AShooterAIController, AShooterNPC, FStateTreeFaceActorInstanceData, ActorToFaceTowards, Controller, FStateTreeFaceLocationInstanceData (+33 more)

### Community 4 - "ShooterPickup.cpp"
Cohesion: 0.06
Nodes (29): FTableRowBase, AShooterPickup(), AShooterPickup::EndPlay(), AShooterPickup::OnConstruction(), AShooterPickup::OnOverlap(), AShooterWeapon, AActor, FHitResult (+21 more)

### Community 5 - "ShooterWeapon.cpp"
Cohesion: 0.07
Nodes (18): AShooterWeapon::ActivateWeapon(), AShooterWeapon::ApplyLiveADSFromConsole(), AShooterWeapon::ApplyLiveMuzzleFromConsole(), AShooterWeapon::AShooterWeapon(), AShooterWeapon::CalculateProjectileSpawnTransform(), AShooterWeapon::EndPlay(), AShooterWeapon::FireProjectile(), AShooterWeapon::GetADSMeshWorldTransform() (+10 more)

### Community 6 - "BattlefieldTrenchGenerator.cpp"
Cohesion: 0.14
Nodes (30): FRandomStream, FVector2D, ABattlefieldTrenchGenerator::AddBeam(), ABattlefieldTrenchGenerator::BuildBunker(), ABattlefieldTrenchGenerator::BuildContinuousTerrain(), ABattlefieldTrenchGenerator::BuildCorrugatedRoof(), ABattlefieldTrenchGenerator::BuildCrater(), ABattlefieldTrenchGenerator::BuildEmplacement() (+22 more)

### Community 7 - "ShooterAIController.cpp"
Cohesion: 0.07
Nodes (17): FEnvQueryContextData, FEnvQueryInstance, UEnvQueryContext_Target::ProvideContext(), AShooterAIController::OnPerceptionForgotten(), AShooterAIController::OnPerceptionUpdated(), AShooterAIController::OnPossess(), AShooterAIController::SetCurrentTarget(), AActor (+9 more)

### Community 8 - "ShooterCharacter.cpp"
Cohesion: 0.07
Nodes (6): AShooterCharacter::EndPlay(), AShooterCharacter::SetupPlayerInputComponent(), AShooterCharacter::UpdateWeaponHUD(), int32, Type, UInputComponent

### Community 9 - "AMyProject7CameraManager"
Cohesion: 0.08
Nodes (26): APlayerCameraManager, AMyProject7CameraManager, AccelOffset, bInitialized, BobPhase, Bodycam, BodycamLensMaterial, BodycamLensMID (+18 more)

### Community 10 - "AMyProject7Character"
Cohesion: 0.13
Nodes (20): AMyProject7Character, DoAim, DoJumpEnd, DoJumpStart, DoMove, FirstPersonCameraComponent, FirstPersonMesh, JumpAction (+12 more)

### Community 11 - "ShooterProjectile.cpp"
Cohesion: 0.10
Nodes (15): ACharacter, AShooterProjectile::EndPlay(), AShooterProjectile::ExplosionCheck(), AShooterProjectile::NotifyHit(), AShooterProjectile::ProcessHit(), AShooterProjectile::SetNoiseTag(), AActor, FHitResult (+7 more)

### Community 12 - "HorrorPlayerController.cpp"
Cohesion: 0.12
Nodes (8): AMyProject7Character(), UInputAction, USpotLightComponent, AHorrorPlayerController::AHorrorPlayerController(), AHorrorPlayerController::OnPossess(), APawn, AHorrorCharacter, UHorrorUI::SetupCharacter()

### Community 13 - "ShooterNPC.cpp"
Cohesion: 0.12
Nodes (10): AShooterNPC::EndPlay(), AShooterNPC::PlayFiringMontage(), AShooterNPC::StartShooting(), AShooterNPC::TakeDamage(), AShooterNPC::UpdateWeaponHUD(), AActor, AController, int32 (+2 more)

### Community 14 - "ShooterPlayerController.cpp"
Cohesion: 0.13
Nodes (10): AShooterPlayerController::AShooterPlayerController(), AShooterPlayerController::OnBulletCountUpdated(), AShooterPlayerController::OnPawnDestroyed(), AShooterPlayerController::OnPossess(), AShooterPlayerController::PostInitializeComponents(), AShooterPlayerController::SetTeam(), AActor, APawn (+2 more)

### Community 15 - "ShooterGameMode.cpp"
Cohesion: 0.15
Nodes (8): AShooterGameMode::ChoosePlayerStart_Implementation(), AShooterGameMode::IncrementTeamScore(), AActor, AController, uint8, AShooterCharacter, UInputMappingContext, UShooterBulletCounterUI

### Community 16 - "Backup_RiderSetup_20260902_224828/MyProject7.sln"
Cohesion: 0.25
Nodes (4): MyProject7, UE5, MyProject7, UE5

### Community 17 - "MyProject7PlayerController.cpp"
Cohesion: 0.20
Nodes (3): AMyProject7PlayerController::AMyProject7PlayerController(), UInputMappingContext, UUserWidget

### Community 18 - "HorrorCharacter.cpp"
Cohesion: 0.20
Nodes (5): AHorrorCharacter::AHorrorCharacter(), AHorrorCharacter::EndPlay(), AHorrorCharacter::SetupPlayerInputComponent(), Type, UInputComponent

### Community 19 - "UnrealBuildTool"
Cohesion: 0.25
Nodes (6): UnrealBuildTool, ModuleRules, MyProject7, MyProject7Target, MyProject7EditorTarget, TargetRules

### Community 20 - "AShooterWeapon"
Cohesion: 0.25
Nodes (9): AShooterNPC::AddWeaponClass(), AShooterNPC::OnWeaponActivated(), AShooterNPC::OnWeaponDeactivated(), TSubclassOf, AShooterCharacter::AddWeaponClass(), AShooterCharacter::FindWeaponOfType(), AShooterCharacter::OnWeaponDeactivated(), TSubclassOf (+1 more)

### Community 21 - ".GetFirstPersonCameraComponent"
Cohesion: 0.29
Nodes (7): AShooterNPC::GetWeaponTargetLocation(), FVector, AShooterCharacter::ApplyLiveADSArmsFromConsole(), AShooterCharacter::ApplyLiveArmsFromConsole(), AShooterCharacter::GetWeaponTargetLocation(), AShooterCharacter::ToggleFlashlight(), FVector

### Community 22 - "MyProject7CameraManager.cpp"
Cohesion: 0.48
Nodes (5): FTViewTarget, AddShotImpulse, ApplyBodycam, UpdateViewTarget, HasEquippedPistol()

### Community 23 - "build_project7_exterior_phase2_lighting.py"
Cohesion: 0.48
Nodes (5): light_component(), mark(), point_light(), set_prop(), static_mesh()

### Community 24 - ".GetFirstPersonMesh"
Cohesion: 0.29
Nodes (6): AShooterNPC::AttachWeaponMeshes(), AShooterCharacter::AttachWeaponMeshes(), AShooterCharacter::OnWeaponActivated(), AShooterCharacter::PlayFiringMontage(), AShooterCharacter::UpdateADSArms(), UAnimMontage

### Community 25 - "ShooterCharacter.h"
Cohesion: 0.29
Nodes (6): AShooterWeapon, FDamageEvent, MYPROJECT7_API, UInputAction, UInputComponent, UPawnNoiseEmitterComponent

### Community 26 - "AHorrorGameMode::ChoosePlayerStart_Implementation"
Cohesion: 0.33
Nodes (3): AHorrorGameMode::ChoosePlayerStart_Implementation(), AActor, AController

### Community 29 - "build_project7_exterior_phase3_detail.py"
Cohesion: 0.60
Nodes (3): decal(), mark(), static_mesh()

### Community 30 - "ShooterNPCSpawner.h"
Cohesion: 0.40
Nodes (3): AShooterNPC, UArrowComponent, UCapsuleComponent

### Community 34 - "AShooterCharacter::TakeDamage"
Cohesion: 0.67
Nodes (3): AShooterCharacter::TakeDamage(), AActor, AController

## Knowledge Gaps
- **162 isolated node(s):** `UInputComponent`, `UPrimitiveComponent`, `UProjectileMovementComponent`, `USphereComponent`, `UInputAction` (+157 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 349 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **5 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `FBodycamCameraSettings` connect `FBodycamCameraSettings` to `AMyProject7CameraManager`, `MyProject7PlayerController.cpp`?**
  _High betweenness centrality (0.103) - this node is a cross-community bridge._
- **Why does `AMyProject7CameraManager` connect `AMyProject7CameraManager` to `FBodycamCameraSettings`, `HorrorPlayerController.cpp`, `ShooterPlayerController.cpp`, `MyProject7PlayerController.cpp`, `MyProject7CameraManager.cpp`?**
  _High betweenness centrality (0.086) - this node is a cross-community bridge._
- **Why does `ACharacter` connect `ShooterProjectile.cpp` to `AMyProject7Character`, `MyProject7CameraManager.cpp`?**
  _High betweenness centrality (0.069) - this node is a cross-community bridge._
- **What connects `UInputComponent`, `UPrimitiveComponent`, `UProjectileMovementComponent` to the rest of the system?**
  _162 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `FBodycamCameraSettings` be split into smaller, more focused modules?**
  _Cohesion score 0.027777777777777776 - nodes in this community are weakly interconnected._
- **Should `Automation_MyProject7.sln` be split into smaller, more focused modules?**
  _Cohesion score 0.12825166364186327 - nodes in this community are weakly interconnected._
- **Should `ShooterStateTreeUtility.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.10048309178743961 - nodes in this community are weakly interconnected._