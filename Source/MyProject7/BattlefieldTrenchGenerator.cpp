#include "BattlefieldTrenchGenerator.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace BattlefieldGeometry
{
	constexpr float PrimitiveSize = 100.0f;

	FTransform Box(const FVector& Location, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		return FTransform(Rotation, Location, Size / PrimitiveSize);
	}

	float DistanceToRoute(const FVector2D& Point, const TArray<FVector>& Route)
	{
		float Result = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index + 1 < Route.Num(); ++Index)
		{
			const FVector Query(Point.X, Point.Y, 0.0f);
			const FVector A(Route[Index].X, Route[Index].Y, 0.0f);
			const FVector B(Route[Index + 1].X, Route[Index + 1].Y, 0.0f);
			Result = FMath::Min(Result, FMath::PointDistToSegment(Query, A, B));
		}
		return Result;
	}

	float GroundHeight(const FVector2D& Point)
	{
		// This must exactly match the unexcavated height in
		// BuildContinuousTerrain.  Keeping a single height contract prevents
		// foliage and scanned props from floating above the ground.
		return (FMath::PerlinNoise2D(Point * 0.00048f) * 24.0f
			+ FMath::PerlinNoise2D(Point * 0.0017f) * 7.0f) * 0.45f;
	}

	float CraterOffset(const FVector2D& Point)
	{
		// Broad, shallow shell craters are sculpted into the continuous terrain.
		// This replaces the old visible sphere stand-ins.
		const TArray<TPair<FVector2D, float>> Craters = {
			TPair<FVector2D, float>(FVector2D(-5800,-5200),420.0f), TPair<FVector2D, float>(FVector2D(-3900,-4700),360.0f),
			TPair<FVector2D, float>(FVector2D(-2200,5100),470.0f), TPair<FVector2D, float>(FVector2D(1550,5200),390.0f),
			TPair<FVector2D, float>(FVector2D(5300,5200),440.0f), TPair<FVector2D, float>(FVector2D(5100,-4700),350.0f),
			TPair<FVector2D, float>(FVector2D(1400,-5850),460.0f), TPair<FVector2D, float>(FVector2D(-6000,4100),400.0f),
			TPair<FVector2D, float>(FVector2D(3800,800),330.0f), TPair<FVector2D, float>(FVector2D(-3100,-1100),380.0f)
		};
		float Offset = 0.0f;
		for (const TPair<FVector2D, float>& Crater : Craters)
		{
			const float NormalizedDistance = FVector2D::Distance(Point, Crater.Key) / Crater.Value;
			if (NormalizedDistance < 1.0f)
			{
				const float Bowl = -FMath::Pow(1.0f - NormalizedDistance, 1.65f) * 92.0f;
				const float Rim = FMath::Exp(-FMath::Square((NormalizedDistance - 0.84f) * 9.0f)) * 13.0f;
				Offset += Bowl + Rim;
			}
		}
		return Offset;
	}

	// The authored TrenchDepth/TrenchWidth are conservative reference figures.
	// Multiply them here so every consumer (terrain mesh, floor props, grounded
	// instances) agrees on the real, deeper/narrower war-trench cross-section.
	constexpr float DepthScale = 1.42f;   // ~210 -> ~300 cm deep
	constexpr float WidthScale = 0.78f;   // ~330 -> ~258 cm nominal

	// Uneven, water-logged trench floor.  Small deterministic noise so duckboards
	// and puddles sit on the same surface the terrain mesh produces.
	float TrenchFloorZ(const FVector2D& Point, const float TrenchDepth)
	{
		return -TrenchDepth * DepthScale
			+ (GroundHeight(Point) / 0.45f) * 0.10f
			+ FMath::PerlinNoise2D(Point * 0.014f) * 13.0f;
	}

	// The single trench cross-section contract.  BuildContinuousTerrain (the
	// collision mesh) and every grounded prop sample this exact profile so nothing
	// floats and the walls read identically to the geometry.  Returns height
	// BEFORE CraterOffset; callers add craters.
	float TrenchProfile(const FVector2D& Point, const float Distance, const float TrenchWidth, const float TrenchDepth)
	{
		const float Field = GroundHeight(Point);                 // undisturbed ground
		const float W = TrenchWidth * WidthScale;

		// Irregular lip: crumbled edges and small collapses instead of a clean bevel.
		const float EdgeNoise =
			  FMath::PerlinNoise2D(Point * 0.0090f) * 24.0f
			+ FMath::PerlinNoise2D(Point * 0.0270f) * 9.0f;

		const float FlatFloorRadius  = W * 0.30f;
		const float WallOuterRadius  = W * 0.60f + EdgeNoise;            // steep + narrow
		const float ParapetOuter     = WallOuterRadius + 165.0f;

		if (Distance >= ParapetOuter)
		{
			return Field;
		}
		if (Distance >= WallOuterRadius)
		{
			// Spoil parapet: earth heaped along the fighting lip.
			const float T = (Distance - WallOuterRadius) / (ParapetOuter - WallOuterRadius);
			return Field + FMath::Sin(T * PI) * (36.0f + EdgeNoise * 0.4f);
		}

		const float Alpha = FMath::Clamp(
			(Distance - FlatFloorRadius) / FMath::Max(WallOuterRadius - FlatFloorRadius, 1.0f), 0.0f, 1.0f);
		// Near-vertical revetment for most of the height, rolling over only at the top.
		const float Wall = FMath::Pow(Alpha, 3.0f);
		return FMath::Lerp(TrenchFloorZ(Point, TrenchDepth), Field, Wall);
	}

	float SurfaceHeight(const FVector2D& Point, const TArray<TArray<FVector>>& Routes, const float TrenchWidth, const float TrenchDepth)
	{
		float Distance = TNumericLimits<float>::Max();
		for (const TArray<FVector>& Route : Routes)
		{
			Distance = FMath::Min(Distance, DistanceToRoute(Point, Route));
		}
		return TrenchProfile(Point, Distance, TrenchWidth, TrenchDepth) + CraterOffset(Point);
	}

	float MeshBottomZ(const UStaticMesh* Mesh, const FRotator& Rotation, const FVector& Scale)
	{
		if (!Mesh)
		{
			return 0.0f;
		}
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		const FQuat Orientation = Rotation.Quaternion();
		float LowestPoint = TNumericLimits<float>::Max();
		for (int32 XSign : {-1, 1})
		{
			for (int32 YSign : {-1, 1})
			{
				for (int32 ZSign : {-1, 1})
				{
					const FVector LocalCorner = Bounds.Origin + FVector(
						Bounds.BoxExtent.X * XSign, Bounds.BoxExtent.Y * YSign, Bounds.BoxExtent.Z * ZSign);
					const FVector ScaledCorner(LocalCorner.X * Scale.X, LocalCorner.Y * Scale.Y, LocalCorner.Z * Scale.Z);
					LowestPoint = FMath::Min(LowestPoint, Orientation.RotateVector(ScaledCorner).Z);
				}
			}
		}
		return LowestPoint;
	}
}

ABattlefieldTrenchGenerator::ABattlefieldTrenchGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("BattlefieldRoot")));
	RootComponent->SetMobility(EComponentMobility::Static);

	auto CreateHISM = [this](const TCHAR* Name)
	{
		auto* Component = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(RootComponent);
		return Component;
	};

	Terrain = CreateHISM(TEXT("Terrain"));
	TerrainSurface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainSurface"));
	TerrainSurface->SetupAttachment(RootComponent);
	OuterForestSurface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OuterForestSurface"));
	OuterForestSurface->SetupAttachment(RootComponent);
	CorrugatedRoofA = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CorrugatedRoofA"));
	CorrugatedRoofB = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CorrugatedRoofB"));
	CorrugatedRoofC = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CorrugatedRoofC"));
	for (UProceduralMeshComponent* Roof : {CorrugatedRoofA.Get(), CorrugatedRoofB.Get(), CorrugatedRoofC.Get()})
	{
		Roof->SetupAttachment(RootComponent);
		Roof->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Roof->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Roof->SetCastShadow(true);
	}
	EarthWalls = CreateHISM(TEXT("EarthWalls"));
	TrenchFloors = CreateHISM(TEXT("TrenchFloors"));
	Timbers = CreateHISM(TEXT("Timbers"));
	Duckboards = CreateHISM(TEXT("Duckboards"));
	Sandbags = CreateHISM(TEXT("Sandbags"));
	CraterRims = CreateHISM(TEXT("CraterRims"));
	Puddles = CreateHISM(TEXT("Puddles"));
	DeadTrees = CreateHISM(TEXT("DeadTrees"));
	GiantBranches = CreateHISM(TEXT("GiantBranches"));
	DeadSnags = CreateHISM(TEXT("DeadSnags"));
	DeadShrubs = CreateHISM(TEXT("DeadShrubs"));
	DeadOaks = CreateHISM(TEXT("DeadOaks"));
	FallenTrees = CreateHISM(TEXT("FallenTrees"));
	WornSandbagBarriers = CreateHISM(TEXT("WornSandbagBarriers"));
	QuarryTires = CreateHISM(TEXT("QuarryTires"));
	ForestRockGroups = CreateHISM(TEXT("ForestRockGroups"));
	RustedMetal = CreateHISM(TEXT("RustedMetal"));
	Crates = CreateHISM(TEXT("Crates"));
	Rubble = CreateHISM(TEXT("Rubble"));
	Bunkers = CreateHISM(TEXT("Bunkers"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Terrain->SetStaticMesh(Cube.Object);
	EarthWalls->SetStaticMesh(Cube.Object);
	TrenchFloors->SetStaticMesh(Cube.Object);
	Timbers->SetStaticMesh(Cube.Object);
	Duckboards->SetStaticMesh(Cube.Object);
	Sandbags->SetStaticMesh(Sphere.Object);
	CraterRims->SetStaticMesh(Sphere.Object);
	Puddles->SetStaticMesh(Cylinder.Object);
	DeadTrees->SetStaticMesh(Cylinder.Object);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BranchScan(TEXT("/Game/Battlefield/Scans/Branches/SM_Branch_Giant.SM_Branch_Giant"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SnagScan(TEXT("/Game/Battlefield/Scans/Snags/SM_DeadSnag.SM_DeadSnag"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShrubScan(TEXT("/Game/Battlefield/Scans/Shrubs/SM_DeadShrub.SM_DeadShrub"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DeadOakScan(TEXT("/Game/Battlefield/Scans/DeadOak/SM_DeadOak.SM_DeadOak"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FallenTreeScan(TEXT("/Game/Battlefield/Scans/FallenTrees/wd3hfidbw_tier_0/StaticMeshes/SM_FallenTree.SM_FallenTree"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SandbagBarrierScan(TEXT("/Game/Battlefield/Scans/Sandbags/SM_ydxlcck_tier_1/StaticMeshes/SM_WornSandbagBarrier.SM_WornSandbagBarrier"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TireScan(TEXT("/Game/Battlefield/Scans/Tires/quarry_tire_scan/StaticMeshes/SM_QuarryTire.SM_QuarryTire"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ForestRockScan(TEXT("/Game/Fab/Forest_ground_rock_group/forest_floor_rock_1/StaticMeshes/forest_floor_rock_1.forest_floor_rock_1"));
	GiantBranches->SetStaticMesh(BranchScan.Object);
	DeadSnags->SetStaticMesh(SnagScan.Object);
	DeadShrubs->SetStaticMesh(ShrubScan.Object);
	DeadOaks->SetStaticMesh(DeadOakScan.Object);
	FallenTrees->SetStaticMesh(FallenTreeScan.Object);
	WornSandbagBarriers->SetStaticMesh(SandbagBarrierScan.Object);
	QuarryTires->SetStaticMesh(TireScan.Object);
	ForestRockGroups->SetStaticMesh(ForestRockScan.Object);
	RustedMetal->SetStaticMesh(Cylinder.Object);
	Crates->SetStaticMesh(Cube.Object);
	Rubble->SetStaticMesh(Cube.Object);
	Bunkers->SetStaticMesh(Cube.Object);

	auto FindMaterial = [](const TCHAR* Path) -> UMaterialInterface*
	{
		return ConstructorHelpers::FObjectFinder<UMaterialInterface>(Path).Object;
	};
	UMaterialInterface* Mud = FindMaterial(TEXT("/Game/Battlefield/Materials/MI_Mud.MI_Mud"));
	UMaterialInterface* ForestGround = FindMaterial(TEXT("/Game/ScansLibrary/Surfaces/M_Forest_Ground_UCn458/Material/M_Forest_Ground_UCn458.M_Forest_Ground_UCn458"));
	UMaterialInterface* GroundBlend = FindMaterial(TEXT("/Game/Battlefield/Materials/M_GroundExteriorInterior.M_GroundExteriorInterior"));
	UMaterialInterface* Wood = FindMaterial(TEXT("/Game/Battlefield/Materials/MI_WetWood.MI_WetWood"));
	UMaterialInterface* Water = FindMaterial(TEXT("/Game/Battlefield/Materials/MI_Puddle.MI_Puddle"));
	UMaterialInterface* Rust = FindMaterial(TEXT("/Game/Battlefield/Materials/MI_Rust.MI_Rust"));
	UMaterialInterface* CorrugatedRoof = FindMaterial(TEXT("/Game/Battlefield/Materials/M_CorrugatedRoof.M_CorrugatedRoof"));
	UMaterialInterface* Concrete = FindMaterial(TEXT("/Game/Battlefield/Materials/MI_BunkerConcrete.MI_BunkerConcrete"));
	UMaterialInterface* GhostBranch = FindMaterial(TEXT("/Game/Battlefield/Materials/M_GhostBranch.M_GhostBranch"));
	UMaterialInterface* TireMaterial = FindMaterial(TEXT("/Game/Battlefield/Materials/M_QuarryTireBattlefield.M_QuarryTireBattlefield"));
	Terrain->SetMaterial(0, Mud);
	TerrainSurface->SetMaterial(0, GroundBlend ? GroundBlend : (ForestGround ? ForestGround : Mud));
	// El apron exterior es demasiado grande para el blend interior/exterior de
	// GroundBlend (genera un patron tipo tablero de ajedrez a esa escala); usa
	// directamente la tierra de bosque.
	OuterForestSurface->SetMaterial(0, ForestGround ? ForestGround : (GroundBlend ? GroundBlend : Mud));
	for (UProceduralMeshComponent* Roof : {CorrugatedRoofA.Get(), CorrugatedRoofB.Get(), CorrugatedRoofC.Get()})
	{
		Roof->SetMaterial(0, CorrugatedRoof ? CorrugatedRoof : Rust);
	}
	TerrainSurface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TerrainSurface->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	TerrainSurface->SetGenerateOverlapEvents(false);
	TerrainSurface->SetCastShadow(true);
	TerrainSurface->bUseAsyncCooking = false;
	OuterForestSurface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	OuterForestSurface->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	OuterForestSurface->SetGenerateOverlapEvents(false);
	OuterForestSurface->SetCastShadow(true);
	OuterForestSurface->bUseAsyncCooking = false;
	EarthWalls->SetMaterial(0, Mud);
	TrenchFloors->SetMaterial(0, Mud);
	Sandbags->SetMaterial(0, Mud);
	CraterRims->SetMaterial(0, Mud);
	Timbers->SetMaterial(0, Wood);
	Duckboards->SetMaterial(0, Wood);
	DeadTrees->SetMaterial(0, Wood);
	GiantBranches->SetMaterial(0, GhostBranch ? GhostBranch : Wood);
	Crates->SetMaterial(0, Wood);
	Puddles->SetMaterial(0, Water);
	RustedMetal->SetMaterial(0, Rust);
	Rubble->SetMaterial(0, Concrete);
	Bunkers->SetMaterial(0, Concrete);
	QuarryTires->SetMaterial(0, TireMaterial ? TireMaterial : Rust);

	SetupHISM(Terrain, true);
	SetupHISM(EarthWalls, true);
	SetupHISM(TrenchFloors, true);
	SetupHISM(Timbers, true);
	SetupHISM(Duckboards, true);
	SetupHISM(Sandbags, true);
	SetupHISM(CraterRims, true);
	SetupHISM(Puddles, false, false);
	SetupHISM(DeadTrees, true);
	SetupHISM(GiantBranches, true);
	SetupHISM(DeadSnags, true);
	SetupHISM(DeadShrubs, false);
	SetupHISM(DeadOaks, true);
	// The tree canopy is the visual boundary of the new 500 m forest apron.
	// Keep it visible farther than small props, while every tree remains one
	// efficient HISM instance rather than an Actor.
	DeadOaks->InstanceStartCullDistance = 60000;
	DeadOaks->InstanceEndCullDistance = 90000;
	SetupHISM(FallenTrees, true);
	SetupHISM(WornSandbagBarriers, true);
	SetupHISM(QuarryTires, true);
	SetupHISM(ForestRockGroups, true);
	SetupHISM(RustedMetal, false);
	SetupHISM(Crates, true);
	SetupHISM(Rubble, false);
	SetupHISM(Bunkers, true);
}

void ABattlefieldTrenchGenerator::SetupHISM(
	UHierarchicalInstancedStaticMeshComponent* Component, const bool bCollision, const bool bShadow)
{
	Component->SetMobility(EComponentMobility::Static);
	Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Component->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCastShadow(bShadow);
	Component->bAffectDistanceFieldLighting = true;
	Component->bReceivesDecals = true;
	Component->InstanceStartCullDistance = 14000;
	Component->InstanceEndCullDistance = 32000;
}

void ABattlefieldTrenchGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateBattlefield();
}

void ABattlefieldTrenchGenerator::ClearBattlefield()
{
	TerrainSurface->ClearAllMeshSections();
	OuterForestSurface->ClearAllMeshSections();
	CorrugatedRoofA->ClearAllMeshSections();
	CorrugatedRoofB->ClearAllMeshSections();
	CorrugatedRoofC->ClearAllMeshSections();
	for (UHierarchicalInstancedStaticMeshComponent* Component :
		{Terrain.Get(), EarthWalls.Get(), TrenchFloors.Get(), Timbers.Get(), Duckboards.Get(), Sandbags.Get(), CraterRims.Get(),
		 Puddles.Get(), DeadTrees.Get(), GiantBranches.Get(), DeadSnags.Get(), DeadShrubs.Get(), DeadOaks.Get(), FallenTrees.Get(),
		 WornSandbagBarriers.Get(), QuarryTires.Get(), ForestRockGroups.Get(), RustedMetal.Get(), Crates.Get(), Rubble.Get(), Bunkers.Get()})
	{
		Component->ClearInstances();
	}
}

void ABattlefieldTrenchGenerator::GenerateBattlefield()
{
	ClearBattlefield();
	FRandomStream Random(Seed);
	const TArray<TArray<FVector>> Routes = {
		{{-900,-7000,0},{-1300,-5400,0},{-650,-3900,0},{-1150,-2250,0},{-400,-650,0},{-800,950,0},{-100,2550,0},{-520,4300,0},{180,7000,0}},
		{{-7000,-700,0},{-5100,-320,0},{-3450,-900,0},{-1900,-380,0},{-400,-650,0},{1200,-220,0},{2850,-760,0},{4550,-250,0},{7000,-620,0}},
		{{-800,950,0},{-2400,1600,0},{-3900,1320,0},{-5200,2150,0},{-6800,1900,0}},
		{{-100,2550,0},{1500,3200,0},{2900,2850,0},{4250,3850,0},{6400,3500,0}},
		{{-1150,-2250,0},{-2600,-3050,0},{-4000,-2750,0},{-5400,-3950,0}},
		{{1200,-220,0},{1750,-1850,0},{2850,-3050,0},{2450,-4700,0},{3650,-6350,0}},
		{{-3900,1320,0},{-4250,3050,0},{-3500,4700,0},{-4500,6400,0}},
		{{2850,-760,0},{4100,250,0},{4750,1700,0},{6400,2150,0}},
		{{-3450,-900,0},{-3250,-2050,0},{-4600,-2500,0},{-5200,-3950,0}},
		{{1750,-1850,0},{620,-2800,0},{-650,-3900,0}},
		{{1500,3200,0},{850,4380,0},{180,5600,0},{-4500,6400,0}},
		{{4100,250,0},{5400,-850,0},{6250,-2100,0},{7000,-2650,0}},
		// Northern traversing trench: crosses four existing routes and creates
		// defensible dog-leg intersections instead of a straight corridor.
		{{-6800,5200,0},{-5500,4850,0},{-4500,5400,0},{-3500,4700,0},{-2050,5250,0},{-520,4300,0},{850,4380,0},{2100,5000,0},{3650,4550,0},{5100,5050,0},{6800,4700,0}},
		// Southern traverse links the western and eastern communication lines.
		{{-6800,-5350,0},{-5600,-4850,0},{-5200,-3950,0},{-4000,-2750,0},{-2650,-3550,0},{-1300,-5400,0},{150,-5050,0},{2450,-4700,0},{3650,-6350,0},{5150,-5550,0},{6800,-5000,0}},
		// Western and eastern spines keep the perimeter connected to the center.
		{{-6250,-6200,0},{-5700,-5000,0},{-5400,-3950,0},{-5900,-2700,0},{-5100,-320,0},{-5750,900,0},{-5200,2150,0},{-5850,3400,0},{-5500,4850,0},{-6200,6350,0}},
		{{6200,-6200,0},{5600,-4800,0},{6250,-2100,0},{5400,-850,0},{4550,-250,0},{5450,850,0},{6400,2150,0},{5650,3300,0},{5100,5050,0},{6200,6350,0}},
		// Two diagonal communication trenches cross at the central network and
		// supply short alternate paths if a main junction is blocked.
		{{-6100,-4550,0},{-4700,-3450,0},{-3250,-2050,0},{-1900,-380,0},{-800,950,0},{1500,3200,0},{3300,4300,0},{5100,5050,0}},
		{{-6000,3900,0},{-4250,3050,0},{-2400,1600,0},{-400,-650,0},{1750,-1850,0},{2850,-3050,0},{4700,-4200,0},{6500,-5200,0}}
	};

	// A single continuous procedural surface replaces the visibly separated
	// terrain cubes.  The height field excavates the whole trench network.
	BuildContinuousTerrain(Routes);
	BuildOuterForestGround();
	// Every scanned prop uses the same sampled terrain height and the true
	// lowest point of its rotated bounds.  This is the shared anti-floating
	// contract for the level; do not reintroduce hard-coded world Z values.
	auto AddGroundedInstance = [this, &Routes](UHierarchicalInstancedStaticMeshComponent* Component,
		const FVector2D& XY, const FRotator& Rotation, const FVector& Scale, const float Embed = 12.0f)
	{
		// Sit on the LOWEST ground under the footprint, not just the pivot point,
		// so nothing is left suspended over an undulation; then dig the base in by
		// Embed so it reads as planted, never hovering.
		float Ground = BattlefieldGeometry::SurfaceHeight(XY, Routes, TrenchWidth, TrenchDepth);
		for (const FVector2D& Sample : {FVector2D(50,0), FVector2D(-50,0), FVector2D(0,50),
			FVector2D(0,-50), FVector2D(35,35), FVector2D(-35,-35)})
		{
			Ground = FMath::Min(Ground, BattlefieldGeometry::SurfaceHeight(XY + Sample, Routes, TrenchWidth, TrenchDepth));
		}
		const float Bottom = BattlefieldGeometry::MeshBottomZ(Component->GetStaticMesh(), Rotation, Scale);
		Component->AddInstance(FTransform(Rotation, FVector(XY.X, XY.Y, Ground - Bottom - Embed), Scale));
	};

	for (int32 Index = 0; Index < Routes.Num(); ++Index)
	{
		BuildRoute(Routes[Index], Random, Index < 2);
	}

	for (const FVector& Point : TArray<FVector>{{-800,950,0},{2850,-760,0},{-3900,1320,0},{4750,1700,0}})
	{
		BuildEmplacement(FVector(Point.X, Point.Y, -TrenchDepth), Random.FRandRange(-180,180), Random);
	}
	// Three improvised positions use actual worn sandbag scans and corrugated
	// sheet roofs.  No basic-shape bunker cubes are left visible in the map.
	struct FBunkerSite { FVector2D XY; float Yaw; UProceduralMeshComponent* Roof; FVector2D RoofSize; float Pitch; };
	for (const FBunkerSite& Site : TArray<FBunkerSite>{
		{{-2450,1620},20,CorrugatedRoofA,FVector2D(390,280),-9},
		{{2900,-3070},125,CorrugatedRoofB,FVector2D(350,250),8},
		{{4250,3880},25,CorrugatedRoofC,FVector2D(365,265),-7}})
	{
		const FRotator Rotation(0, Site.Yaw, 0);
		const FVector Forward = Rotation.Vector();
		const FVector Right = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y);
		for (const FVector& Offset : TArray<FVector>{Forward * 195.0f, Right * 190.0f, -Right * 190.0f})
		{
			const FVector2D BarrierXY(Site.XY.X + Offset.X, Site.XY.Y + Offset.Y);
			AddGroundedInstance(WornSandbagBarriers, BarrierXY, Rotation, FVector(0.90f), 10.0f);
		}
		// Roof the dugout at the parapet lip, not at the (now much deeper) floor.
		const float RoofZ = BattlefieldGeometry::GroundHeight(Site.XY) + Random.FRandRange(38.0f, 58.0f);
		BuildCorrugatedRoof(Site.Roof, FVector(Site.XY.X, Site.XY.Y, RoofZ), Site.RoofSize, Site.Yaw, Site.Pitch);
	}

	// Timber posts + horizontal planking are kept: they are the wall revetment
	// frame that makes the trench read as built, not just a dug ditch.  Only the
	// broad branch scan is dropped (its bounds read as a floating plank close up).
	GiantBranches->ClearInstances();

	// The exterior is a hostile, broken forest.  The user's original dead-oak
	// scan is used untouched, clustered organically through HISM rather than
	// copied as hundreds of independent actors.
	const TArray<FVector2D> OakGroves = {
		{-4200,-2650},{-3600,450},{-2800,3250},{-800,4300},{1450,4050},{3650,3050},{4700,900},
		{3900,-2750},{1300,-3850},{-1500,-3500},{-4700,-900},{-5350,2350}
	};
	if (DeadOaks->GetStaticMesh())
	{
		auto AddGroundedOak = [this, &AddGroundedInstance](const FVector2D& XY, const FRotator& Rotation, const FVector& Scale)
		{
			// Trunks bury a good half-metre so they never stand on a visible stub.
			AddGroundedInstance(DeadOaks, XY, Rotation, Scale, 55.0f);
		};
		int32 Placed = 0;
		for (int32 Attempt = 0; Attempt < 460 && Placed < 145; ++Attempt)
		{
			const FVector2D Grove = OakGroves[Random.RandRange(0, OakGroves.Num() - 1)];
			const float Radius = FMath::Sqrt(Random.FRand()) * Random.FRandRange(380.0f, 1350.0f);
			const float Angle = Random.FRandRange(0.0f, 2.0f * PI);
			const FVector2D XY(Grove.X + FMath::Cos(Angle) * Radius, Grove.Y + FMath::Sin(Angle) * Radius);
			float RouteDistance = TNumericLimits<float>::Max();
			for (const TArray<FVector>& Route : Routes)
			{
				RouteDistance = FMath::Min(RouteDistance, BattlefieldGeometry::DistanceToRoute(XY, Route));
			}
			// Preserve walking space and sightlines inside every trench route.
			if (RouteDistance < 740.0f)
			{
				continue;
			}
			// Source OBJ is authored in metres but imported as centimetres.  Scale
			// x100 restores a believable 5-20 m old-oak range in first person.
			const float Scale = Random.FRandRange(48.0f, 142.0f);
			const FRotator Rotation(
				// OBJ is Y-up; rotate it once into Unreal's Z-up convention, then
				// add a very restrained organic lean so trunks read as rooted, not
				// knocked over.
				Random.FRandRange(-3, 3), Random.FRandRange(-180, 180), 90.0f + Random.FRandRange(-4, 4));
			AddGroundedOak(XY, Rotation, FVector(Scale, Scale * Random.FRandRange(0.88f, 1.12f), Scale));
			++Placed;
		}

		// The central 140 m field is retained for the shooting and trench layout.
		// These distant instances populate the new 500 m outer apron without
		// creating individual Actors or obstructing trench sightlines.
		int32 OuterPlaced = 0;
		for (int32 Attempt = 0; Attempt < 5600 && OuterPlaced < 1350; ++Attempt)
		{
			const FVector2D XY(Random.FRandRange(-OuterForestHalfExtent + 1000.0f, OuterForestHalfExtent - 1000.0f),
				Random.FRandRange(-OuterForestHalfExtent + 1000.0f, OuterForestHalfExtent - 1000.0f));
			if (FMath::Abs(XY.X) < HalfExtent + 500.0f && FMath::Abs(XY.Y) < HalfExtent + 500.0f)
			{
				continue;
			}
			const float Scale = Random.FRandRange(54.0f, 135.0f);
			const FRotator Rotation(Random.FRandRange(-3, 3), Random.FRandRange(-180, 180), 90.0f + Random.FRandRange(-4, 4));
			AddGroundedOak(XY, Rotation, FVector(Scale, Scale * Random.FRandRange(0.9f, 1.1f), Scale));
			++OuterPlaced;
		}
	}
	for (const FVector2D& XY : TArray<FVector2D>{{-5700,-4450},{-5050,-3380},{-4450,4140},{-3500,4850},{-1750,5130},{1420,4990},{4700,4150},{5600,-3680},{-6100,-1100},{4100,-780}})
	{
		if (ForestRockGroups->GetStaticMesh())
		{
			const float Scale = Random.FRandRange(0.62f, 1.25f);
			AddGroundedInstance(ForestRockGroups, XY, FRotator(Random.FRandRange(-4, 4), Random.FRandRange(-180, 180), Random.FRandRange(-4, 4)), FVector(Scale), 26.0f);
		}
	}

	// Other scans make the edge of the oak forest look old, dead and uneven.
	// Silver-fir source pieces remain available in Content but are deliberately
	// omitted: their separated geometry does not read as a complete tree.
	// The imported shrubs are retained in Content for later use, but are not
	// placed inside trenches.  They fill only the outer dead forest floor.
	if (DeadShrubs->GetStaticMesh())
	{
		int32 ShrubsPlaced = 0;
		for (int32 Attempt = 0; Attempt < 700 && ShrubsPlaced < 180; ++Attempt)
		{
			const FVector2D XY(Random.FRandRange(-6500,6500), Random.FRandRange(-6500,6500));
			float RouteDistance = TNumericLimits<float>::Max();
			for (const TArray<FVector>& Route : Routes) RouteDistance = FMath::Min(RouteDistance, BattlefieldGeometry::DistanceToRoute(XY, Route));
			if (RouteDistance < 1150.0f) continue;
			const float Scale = Random.FRandRange(0.38f, 0.92f);
			AddGroundedInstance(DeadShrubs, XY, FRotator(0,Random.FRandRange(-180,180),0), FVector(Scale), 14.0f);
			++ShrubsPlaced;
		}
	}
	// This imported shrub has an unresolved source material in this project
	// (it renders with red/green artefacts).  Keep the asset but do not place
	// it until its authored material is corrected; broken foliage is worse than
	// a clean, dense scanned-oak forest.
	DeadShrubs->ClearInstances();
	for (const FVector2D& Location : TArray<FVector2D>{{-5350,-3850},{-3650,4700},{5250,1800},{3900,-4950},{-1850,5250}})
	{
		if (FallenTrees->GetStaticMesh())
		{
			const float Scale = Random.FRandRange(0.7f, 1.25f);
			AddGroundedInstance(FallenTrees, Location, FRotator(Random.FRandRange(-6, 6), Random.FRandRange(-180, 180), Random.FRandRange(-6, 6)), FVector(Scale), 22.0f);
		}
	}
	// Exactly two modest, real scanned fallen trunks lie on the trench floor.
	// They are grounded through the same sampled surface contract as the forest.
	for (const FVector2D& Location : TArray<FVector2D>{{-620,-500},{420,-400}})
	{
		if (FallenTrees->GetStaticMesh())
		{
			AddGroundedInstance(FallenTrees, Location,
				FRotator(Random.FRandRange(-3, 3), Random.FRandRange(-180, 180), Random.FRandRange(-3, 3)), FVector(0.40f), 16.0f);
		}
	}
	// Deliberately sparse junk clusters read as abandoned positions, rather than
	// an evenly scattered prop field.
	for (const FVector2D& Location : TArray<FVector2D>{{-2700,-2380},{2550,-2820},{-4250,3020}})
	{
		if (QuarryTires->GetStaticMesh())
		{
			const int32 Count = Random.RandRange(1, 2);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const FVector2D OffsetLocation(Location.X + Random.FRandRange(-110, 110), Location.Y + Random.FRandRange(-110, 110));
				const float Scale = Random.FRandRange(0.65f, 1.05f);
				AddGroundedInstance(QuarryTires, OffsetLocation, FRotator(Random.FRandRange(-14, 14), Random.FRandRange(-180, 180), Random.FRandRange(-60, 60)), FVector(Scale), 9.0f);
			}
		}
	}
	for (const FVector2D& Location : TArray<FVector2D>{
		{-820,950},{2800,-760},{-3900,1300},{4760,1700},{-3350,-2020},{1300,3190},
		{-1700,-2350},{-2600,1700},{2200,1800},{3550,250},{-5100,-300},{5200,-1100},
		{-4550,3200},{900,4400},{-1200,3200},{4700,2100},{-3600,-3300},{3100,-3200},
		// New crossing strongpoints, spaced away from the actual walking center.
		{-5470,4800},{-3630,4770},{2020,4920},{5100,5000},
		{-5550,-4760},{-4020,-2860},{2440,-4580},{5120,-5450},
		{-5750,880},{5450,820},{-2380,1490},{1730,-1740}})
	{
		if (WornSandbagBarriers->GetStaticMesh())
		{
			const float Scale = Random.FRandRange(0.85f, 1.10f);
			AddGroundedInstance(WornSandbagBarriers, Location, FRotator(0, Random.FRandRange(-180, 180), 0), FVector(Scale), 10.0f);
		}
	}
	// Sphere stand-in bags stay removed; the scanned worn-barrier asset is the
	// only sandbag geometry.  Duckboards (wet-wood plank boxes) and puddles
	// (flat puddle-material discs) ARE kept - they are the trench floor detail.
	Sandbags->ClearInstances();

	BuildWireLine(FVector(-6300,-5000,0), FVector(-3900,-4500,0), Random);
	BuildWireLine(FVector(4200,-5400,0), FVector(6500,-4700,0), Random);
	BuildWireLine(FVector(-6500,3300,0), FVector(-4300,3900,0), Random);
	BuildWireLine(FVector(4100,5100,0), FVector(6500,4700,0), Random);
	// Tension wires bind the existing trench-edge posts into believable field
	// barriers; two staggered lines avoid a perfectly straight artificial look.
	for (const TPair<FVector, FVector>& Span : TArray<TPair<FVector, FVector>>{
		{FVector(-1550,-2150,96), FVector(-620,-1720,104)},
		{FVector(-880,-1380,118), FVector(120,-930,101)},
		{FVector(420,-520,108), FVector(1180,-170,98)},
		{FVector(-1080,1280,104), FVector(-360,1830,115)},
		{FVector(1860,-680,112), FVector(2600,-940,102)}})
	{
		AddBeam(RustedMetal, Span.Key, Span.Value, 1.25f);
		AddBeam(RustedMetal, Span.Key - FVector(0,0,28), Span.Value - FVector(0,0,20), 1.0f);
	}

	// Do not use cube rubble or cube crates as visual props.  The scene remains
	// deliberately sparse until matching scanned military props are available.
}

void ABattlefieldTrenchGenerator::BuildRoute(const TArray<FVector>& Points, FRandomStream& Random, const bool bMain)
{
	for (int32 Segment = 0; Segment + 1 < Points.Num(); ++Segment)
	{
		const FVector A(Points[Segment].X, Points[Segment].Y, 0.0f);
		const FVector B(Points[Segment+1].X, Points[Segment+1].Y, 0.0f);
		const FVector Delta = B-A;
		const FVector Tangent = Delta.GetSafeNormal2D();
		const FVector Normal(-Tangent.Y,Tangent.X,0);
		const float Yaw = Tangent.Rotation().Yaw;
		const int32 Steps = FMath::Max(1,FMath::CeilToInt(Delta.Size2D()/200.0f));
		for (int32 Step=0; Step<=Steps; ++Step)
		{
			const FVector2D CenterXY = FMath::Lerp(FVector2D(A), FVector2D(B), static_cast<float>(Step)/Steps);
			// The real, uneven floor height at this point (matches the collision mesh).
			const float FloorZ = BattlefieldGeometry::TrenchFloorZ(CenterXY, TrenchDepth);
			const FVector Center(CenterXY.X, CenterXY.Y, FloorZ);
			// Walls are near-vertical at ~0.72 * width; revetment sits right against them.
			const float HalfWidth = TrenchWidth*0.5f + Random.FRandRange(-10,18);
			const float WallDist = TrenchWidth*0.70f;

			for (float Side : {-1.0f,1.0f})
			{
				const FVector Foot = Center + Normal*Side*(WallDist-8);
				// Vertical timber posts every other step: the wall revetment frame.
				if ((Step+Segment)%2==0)
				{
					AddBeam(Timbers, Foot+FVector(0,0,4), Foot+FVector(0,0,TrenchDepth*0.92f+Random.FRandRange(-8,10)), Random.FRandRange(6,9));
				}
				// Horizontal planking boarding the earth wall (A-frame revetment look).
				if (Random.FRand()<0.55f)
				{
					const int32 Boards = Random.RandRange(2,3);
					for (int32 Board=0; Board<Boards; ++Board)
					{
						const float BZ = FloorZ + 22.0f + Board*(TrenchDepth*0.30f) + Random.FRandRange(-6,6);
						const FVector P0 = Center + Tangent*(-95) + Normal*Side*WallDist + FVector(0,0,BZ-FloorZ);
						const FVector P1 = Center + Tangent*( 95) + Normal*Side*WallDist + FVector(0,0,BZ-FloorZ+Random.FRandRange(-5,5));
						AddBeam(Timbers, P0, P1, Random.FRandRange(4,6));
					}
				}
			}
			// Broken, sagging duckboards over the wet floor.  Frequent but never a
			// perfect carpet: gaps expose the mud and puddles.
			if (Random.FRand()<0.62f)
			{
				Duckboards->AddInstance(BattlefieldGeometry::Box(
					Center+FVector(0,0,7),
					FVector(180,TrenchWidth*0.78f,Random.FRandRange(7,12)),
					FRotator(Random.FRandRange(-3,3),Yaw+Random.FRandRange(-6,6),Random.FRandRange(-4,4))));
			}
			// Standing water in the low spots.
			if (Random.FRand()<0.34f)
			{
				Puddles->AddInstance(FTransform(FRotator::ZeroRotator,
					Center+FVector(0,0,2.5f),
					FVector(Random.FRandRange(1.0f,2.4f),Random.FRandRange(.6f,1.4f),.02f)));
			}
			// Subtle war debris on the floor: broken planks, small stones, rusted
			// scrap.  Low probability so it reads as scattered, not a prop field.
			if (Random.FRand()<0.22f)
			{
				const FVector D = Center + Tangent*Random.FRandRange(-70,70) + Normal*Random.FRandRange(-WallDist*0.55f,WallDist*0.55f);
				Duckboards->AddInstance(BattlefieldGeometry::Box(
					D+FVector(0,0,Random.FRandRange(3,7)),
					FVector(Random.FRandRange(55,130),Random.FRandRange(12,22),Random.FRandRange(5,9)),
					FRotator(Random.FRandRange(-9,9),Random.FRandRange(-180,180),Random.FRandRange(-14,14))));
			}
			if (ForestRockGroups->GetStaticMesh() && Random.FRand()<0.16f)
			{
				const FVector2D RXY(Center.X+Random.FRandRange(-90,90), Center.Y+Random.FRandRange(-90,90));
				const float RS = Random.FRandRange(0.10f,0.26f);
				const float RG = BattlefieldGeometry::TrenchFloorZ(RXY, TrenchDepth);
				const float RB = BattlefieldGeometry::MeshBottomZ(ForestRockGroups->GetStaticMesh(), FRotator::ZeroRotator, FVector(RS));
				ForestRockGroups->AddInstance(FTransform(FRotator(Random.FRandRange(-20,20),Random.FRandRange(-180,180),Random.FRandRange(-20,20)),FVector(RXY.X,RXY.Y,RG-RB-6.0f),FVector(RS)));
			}
			if (Random.FRand()<0.10f)
			{
				const FVector S = Center + Normal*Random.FRandRange(-WallDist*0.6f,WallDist*0.6f) + FVector(0,0,Random.FRandRange(3,8));
				AddBeam(RustedMetal, S, S+FVector(Random.FRandRange(-60,60),Random.FRandRange(-60,60),Random.FRandRange(-4,10)), Random.FRandRange(2.0f,4.0f));
			}
			// Sandbag revetment along the fighting lip of the main routes.
			if (bMain && Random.FRand()<0.5f)
			{
				const float Side=Random.FRand()<.5f?-1.0f:1.0f;
				const float LipZ = FloorZ + TrenchDepth - 16.0f;
				WornSandbagBarriers->AddInstance(FTransform(
					FRotator(0,Yaw+90+Random.FRandRange(-10,10),0),
					Center+Normal*Side*(WallDist-4)+FVector(0,0,LipZ-BattlefieldGeometry::MeshBottomZ(WornSandbagBarriers->GetStaticMesh(),FRotator::ZeroRotator,FVector(0.78f))),
					FVector(0.78f)));
			}
		}
	}
}

void ABattlefieldTrenchGenerator::BuildContinuousTerrain(const TArray<TArray<FVector>>& Routes)
{
	constexpr float GridSpacing = 100.0f;
	const int32 GridCount = FMath::RoundToInt((HalfExtent * 2.0f) / GridSpacing) + 1;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	Vertices.Reserve(GridCount * GridCount);
	UVs.Reserve(GridCount * GridCount);
	Colors.Reserve(GridCount * GridCount);

	for (int32 Y = 0; Y < GridCount; ++Y)
	{
		for (int32 X = 0; X < GridCount; ++X)
		{
			const float WorldX = -HalfExtent + X * GridSpacing;
			const float WorldY = -HalfExtent + Y * GridSpacing;
			const FVector2D Point(WorldX, WorldY);
			float Distance = TNumericLimits<float>::Max();
			for (const TArray<FVector>& Route : Routes)
			{
				Distance = FMath::Min(Distance, BattlefieldGeometry::DistanceToRoute(Point, Route));
			}

			// Same cross-section contract as SurfaceHeight so props never float and
			// the collision matches the visible wall exactly.
			const float Height = BattlefieldGeometry::TrenchProfile(Point, Distance, TrenchWidth, TrenchDepth);
			Vertices.Add(FVector(WorldX, WorldY, Height + BattlefieldGeometry::CraterOffset(Point)));
			UVs.Add(FVector2D(WorldX / 350.0f, WorldY / 350.0f));
			// Red vertex color is a soft mask: Forest Ground is reserved for the
			// protected inner guerrilla/trench area, while exterior uses the mud scan.
			const float BlendStart = TrenchWidth * 1.05f;
			const float BlendEnd = TrenchWidth * 1.65f;
			const float InnerMask = 1.0f - FMath::Clamp((Distance - BlendStart) / (BlendEnd - BlendStart), 0.0f, 1.0f);
			Colors.Add(FLinearColor(InnerMask, 0.0f, 0.0f, 1.0f));
		}
	}

	for (int32 Y = 0; Y + 1 < GridCount; ++Y)
	{
		for (int32 X = 0; X + 1 < GridCount; ++X)
		{
			const int32 A = Y * GridCount + X;
			const int32 B = A + 1;
			const int32 C = A + GridCount;
			const int32 D = C + 1;
			Triangles.Append({A, C, B, B, C, D});
		}
	}

	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
	TerrainSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
}

void ABattlefieldTrenchGenerator::BuildOuterForestGround()
{
	// The main terrain is deliberately high-resolution near the trenches.  A
	// separate, low-frequency outer surface gives the requested 500 m forest
	// buffer without turning the center into a million-vertex collision mesh.
	constexpr float GridSpacing = 2000.0f;
	const float Inner = HalfExtent;
	const float Outer = FMath::Max(OuterForestHalfExtent, Inner + 1000.0f);
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;

	auto AppendStrip = [&Vertices, &Triangles, &UVs, &Colors](const float MinX, const float MaxX, const float MinY, const float MaxY)
	{
		const int32 XCount = FMath::CeilToInt((MaxX - MinX) / GridSpacing) + 1;
		const int32 YCount = FMath::CeilToInt((MaxY - MinY) / GridSpacing) + 1;
		const int32 FirstVertex = Vertices.Num();
		for (int32 Y = 0; Y < YCount; ++Y)
		{
			const float WorldY = FMath::Lerp(MinY, MaxY, static_cast<float>(Y) / (YCount - 1));
			for (int32 X = 0; X < XCount; ++X)
			{
				const float WorldX = FMath::Lerp(MinX, MaxX, static_cast<float>(X) / (XCount - 1));
				const FVector2D Point(WorldX, WorldY);
				Vertices.Add(FVector(WorldX, WorldY, BattlefieldGeometry::GroundHeight(Point)));
				UVs.Add(FVector2D(WorldX / 350.0f, WorldY / 350.0f));
				// Red=0 selects the continuous exterior mud/forest material branch.
				Colors.Add(FLinearColor(0, 0, 0, 1));
			}
		}
		for (int32 Y = 0; Y + 1 < YCount; ++Y)
		{
			for (int32 X = 0; X + 1 < XCount; ++X)
			{
				const int32 A = FirstVertex + Y * XCount + X;
				const int32 B = A + 1;
				const int32 C = A + XCount;
				const int32 D = C + 1;
				Triangles.Append({A, C, B, B, C, D});
			}
		}
	};

	// Four strips form a ring around the high-detail square, avoiding overlap
	// and z-fighting while leaving the trench mesh untouched.
	AppendStrip(-Outer, Outer, Inner, Outer);
	AppendStrip(-Outer, Outer, -Outer, -Inner);
	AppendStrip(-Outer, -Inner, -Inner, Inner);
	AppendStrip(Inner, Outer, -Inner, Inner);

	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
	OuterForestSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
}

void ABattlefieldTrenchGenerator::BuildCorrugatedRoof(UProceduralMeshComponent* Roof, const FVector& Center,
	const FVector2D& HalfSize, const float Yaw, const float Pitch)
{
	constexpr float RibSpacing = 16.0f;
	constexpr float RibHeight = 8.0f;
	const int32 Ribs = FMath::Max(8, FMath::CeilToInt((HalfSize.Y * 2.0f) / RibSpacing));
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	Vertices.Reserve((Ribs + 1) * 2);
	UVs.Reserve((Ribs + 1) * 2);
	Colors.Reserve((Ribs + 1) * 2);
	const FTransform Transform(FRotator(Pitch, Yaw, 0), Center);

	for (int32 Rib = 0; Rib <= Ribs; ++Rib)
	{
		const float Fraction = static_cast<float>(Rib) / Ribs;
		const float LocalY = FMath::Lerp(-HalfSize.Y, HalfSize.Y, Fraction);
		const float Corrugation = FMath::Sin(Fraction * Ribs * PI * 2.0f) * RibHeight;
		for (float LocalX : {-HalfSize.X, HalfSize.X})
		{
			Vertices.Add(Transform.TransformPosition(FVector(LocalX, LocalY, Corrugation)));
			UVs.Add(FVector2D((LocalX + HalfSize.X) / 100.0f, Fraction * Ribs * 0.5f));
			Colors.Add(FLinearColor::White);
		}
	}
	for (int32 Rib = 0; Rib < Ribs; ++Rib)
	{
		const int32 A = Rib * 2;
		Triangles.Append({A, A + 2, A + 1, A + 1, A + 2, A + 3});
	}
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
	Roof->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
}

void ABattlefieldTrenchGenerator::BuildEmplacement(const FVector& Center, const float Yaw, FRandomStream& Random)
{
	for (int32 Row=0; Row<2; ++Row)
	{
		const float Radius=260+Row*65;
		const int32 Count=16+Row*3;
		for(int32 Index=0;Index<Count;++Index)
		{
			const float Angle=-145.0f+290.0f*Index/(Count-1);
			const float Radians=FMath::DegreesToRadians(Angle+Yaw);
			const FVector Location=Center+FVector(FMath::Cos(Radians),FMath::Sin(Radians),0)*Radius+FVector(0,0,TrenchDepth-25+Row*36);
			Sandbags->AddInstance(FTransform(FRotator(0,Angle+Yaw+90+Random.FRandRange(-7,7),0),Location,FVector(.82f,.38f,.27f)));
		}
	}
}

void ABattlefieldTrenchGenerator::BuildBunker(const FVector& Center, const float Yaw, FRandomStream& Random)
{
	// Kept for backward compatibility with existing placed actors.  Bunker
	// construction is now done directly from scanned sandbag barriers above.
}

void ABattlefieldTrenchGenerator::BuildCrater(const FVector& Center, const float Radius, FRandomStream& Random)
{
	// Craters are part of the terrain height field, not sphere rings or flat
	// cylinder puddles.  See BattlefieldGeometry::CraterOffset.
}

void ABattlefieldTrenchGenerator::BuildTree(const FVector& Base, const float Height, FRandomStream& Random)
{
	if (GiantBranches->GetStaticMesh())
	{
		const int32 BranchCount = Random.RandRange(1, 2);
		for (int32 Index = 0; Index < BranchCount; ++Index)
		{
			const FVector Offset(Random.FRandRange(-220, 220), Random.FRandRange(-220, 220), Random.FRandRange(-8, 24));
			const FRotator Rotation(Random.FRandRange(-18, 18), Random.FRandRange(-180, 180), Random.FRandRange(-32, 32));
			const float Scale = Random.FRandRange(1.35f, 2.65f);
			GiantBranches->AddInstance(FTransform(Rotation, Base + Offset, FVector(Scale, Scale, Scale)));
		}
		return;
	}
	const FVector Top=Base+FVector(Random.FRandRange(-70,70),Random.FRandRange(-70,70),Height);
	AddBeam(DeadTrees,Base,Top,Random.FRandRange(18,34));
	for(int32 Index=0;Index<Random.RandRange(2,5);++Index)
	{
		const FVector Start=FMath::Lerp(Base,Top,Random.FRandRange(.43f,.87f));
		AddBeam(DeadTrees,Start,Start+FVector(Random.FRandRange(-200,200),Random.FRandRange(-200,200),Random.FRandRange(70,220)),Random.FRandRange(5,11));
	}
}

void ABattlefieldTrenchGenerator::BuildWireLine(const FVector& Start, const FVector& End, FRandomStream& Random)
{
	FVector PreviousHigh,PreviousLow;
	constexpr int32 Posts=12;
	for(int32 Index=0;Index<Posts;++Index)
	{
		FVector Base=FMath::Lerp(Start,End,static_cast<float>(Index)/(Posts-1))+FVector(Random.FRandRange(-25,25),Random.FRandRange(-25,25),0);
		Base.Z = BattlefieldGeometry::GroundHeight(FVector2D(Base.X, Base.Y));
		AddBeam(RustedMetal,Base,Base+FVector(Random.FRandRange(-15,15),Random.FRandRange(-15,15),175),5);
		const FVector High=Base+FVector(0,0,140+(Index%2?-18:12));
		const FVector Low=Base+FVector(0,0,82+(Index%2?12:-14));
		if(Index>0){AddBeam(RustedMetal,PreviousHigh,High,1.35f);AddBeam(RustedMetal,PreviousLow,Low,1.15f);}
		PreviousHigh=High; PreviousLow=Low;
	}
}

void ABattlefieldTrenchGenerator::AddBeam(UHierarchicalInstancedStaticMeshComponent* Component,const FVector& Start,const FVector& End,const float Radius)
{
	const FVector Delta=End-Start;
	if(Delta.IsNearlyZero()) return;
	const float Length=Delta.Size();
	const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Delta/Length);
	Component->AddInstance(FTransform(Rotation,(Start+End)*.5f,FVector(Radius/50,Radius/50,Length/100)));
}
