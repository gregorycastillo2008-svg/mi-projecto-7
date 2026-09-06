#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattlefieldTrenchGenerator.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;

/** Dense deterministic battlefield built from instanced geometry. */
UCLASS(BlueprintType)
class MYPROJECT7_API ABattlefieldTrenchGenerator : public AActor
{
	GENERATED_BODY()

public:
	ABattlefieldTrenchGenerator();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Battlefield")
	void GenerateBattlefield();

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Battlefield")
	void ClearBattlefield();

protected:
	UPROPERTY(EditAnywhere, Category="Battlefield", meta=(ClampMin="1"))
	int32 Seed = 1917;

	UPROPERTY(EditAnywhere, Category="Battlefield", meta=(ClampMin="5000", Units="cm"))
	float HalfExtent = 7000.0f;

	// A 500 m playable-looking forest apron surrounds the authored trench area.
	// It is deliberately low-frequency geometry so the detailed central trench
	// mesh and its collision remain inexpensive.
	UPROPERTY(EditAnywhere, Category="Battlefield", meta=(ClampMin="10000", Units="cm"))
	float OuterForestHalfExtent = 57000.0f;

	UPROPERTY(EditAnywhere, Category="Battlefield", meta=(ClampMin="260", ClampMax="450", Units="cm"))
	float TrenchWidth = 330.0f;

	UPROPERTY(EditAnywhere, Category="Battlefield", meta=(ClampMin="170", ClampMax="250", Units="cm"))
	float TrenchDepth = 210.0f;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Terrain;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> TerrainSurface;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> OuterForestSurface;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> CorrugatedRoofA;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> CorrugatedRoofB;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> CorrugatedRoofC;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> EarthWalls;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TrenchFloors;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Timbers;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Duckboards;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Sandbags;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CraterRims;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Puddles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DeadTrees;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GiantBranches;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DeadSnags;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DeadShrubs;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DeadOaks;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FallenTrees;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WornSandbagBarriers;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> QuarryTires;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ForestRockGroups;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RustedMetal;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Crates;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Rubble;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Bunkers;

private:
	void SetupHISM(UHierarchicalInstancedStaticMeshComponent* Component, bool bCollision, bool bShadow = true);
	void BuildContinuousTerrain(const TArray<TArray<FVector>>& Routes);
	void BuildOuterForestGround();
	void BuildCorrugatedRoof(UProceduralMeshComponent* Roof, const FVector& Center,
		const FVector2D& HalfSize, float Yaw, float Pitch);
	void BuildRoute(const TArray<FVector>& Points, FRandomStream& Random, bool bMain);
	void BuildEmplacement(const FVector& Center, float Yaw, FRandomStream& Random);
	void BuildBunker(const FVector& Center, float Yaw, FRandomStream& Random);
	void BuildCrater(const FVector& Center, float Radius, FRandomStream& Random);
	void BuildTree(const FVector& Base, float Height, FRandomStream& Random);
	void BuildWireLine(const FVector& Start, const FVector& End, FRandomStream& Random);
	void AddBeam(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Start,
		const FVector& End, float Radius);
};
