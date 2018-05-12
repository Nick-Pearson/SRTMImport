#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SRTMContainer.generated.h"


// Object that stores a loaded rectangle of SRTM data and provides a set of functions for reading the loaded data
UCLASS()
class SRTMIMPORT_API USRTMContainer : public UObject
{
	GENERATED_BODY()

public:

	// the raw SRTM heightmap data
	TArray<int16> RawData;

	// how many values on each side of the rectangle
	UPROPERTY(BlueprintReadOnly, Category = "SRTMContainer")
	FIntPoint DimensionSize;

	UPROPERTY(BlueprintReadOnly, Category = "SRTMContainer")
	FVector2D StartLatLong;

	UPROPERTY(BlueprintReadOnly, Category = "SRTMContainer")
	FVector2D EndLatLong;

public:

	UFUNCTION(BlueprintCallable, Category = "SRTMContainer")
	int32 Get(int32 X, int32 Y) const;

	int16 Get_Raw(int32 X, int32 Y) const;

	FORCEINLINE int16 Get_Unchecked(int32 X, int32 Y) const { return RawData[X + (Y * DimensionSize.X)]; }
};
