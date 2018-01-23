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
	FIntPoint DimensionSize;

	FVector2D StartLatLong;
	FVector2D EndLatLong;
};
