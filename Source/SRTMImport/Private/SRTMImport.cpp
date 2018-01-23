#include "ISRTMImport.h"

#include "SRTMContainer.h"

#include "FileManager.h"
#include "DefaultValueHelper.h"


class FSRTMImport : public ISRTMImport
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	virtual USRTMContainer* LoadFile(const FString& Filepath) const override;
	virtual USRTMContainer* LoadFile(const FString& Filepath, int32 FileLat, int32 FileLong) const override;

	virtual USRTMContainer* LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong, EIncompleteDataHandling HandlingType = EIncompleteDataHandling::RETURN_NULLPTR) const override;

private:

	FString GetFilenameFromLatLong(int32 Lat, int32 Long) const;
	bool GetLatLongFromFilename(const FString& Filename, int32& Lat, int32& Long) const;
};

IMPLEMENT_MODULE( FSRTMImport, SRTMImport )



void FSRTMImport::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FSRTMImport::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

USRTMContainer* FSRTMImport::LoadFile(const FString& Filepath) const
{
	int32 Lat, Long;
	if (!GetLatLongFromFilename(Filepath, Lat, Long)) return nullptr;

	return LoadFile(Filepath, Lat, Long);
}

USRTMContainer* FSRTMImport::LoadFile(const FString& Filepath, int32 FileLat, int32 FileLong) const
{
	TSharedPtr<FArchive> Archive = MakeShareable(IFileManager::Get().CreateFileReader(*Filepath));

	if (!Archive.IsValid())
		return nullptr;

	int64 Filesize = Archive->TotalSize();

	//ensure this is a valid file - must be factor of two and square number
	if (Filesize % 2 == 1) return nullptr;

	int32 numValues = Filesize / 2;
	float dimensionSize_flt = FMath::Sqrt((float)numValues);

	if (!FMath::IsNearlyZero(dimensionSize_flt)) return nullptr;

	int32 dimensionSize = (int32)dimensionSize_flt;

	// fill in the basic container info
	USRTMContainer* ContainerPtr = NewObject<USRTMContainer>();
	ContainerPtr->DimensionSize = FIntPoint(dimensionSize, dimensionSize);
	ContainerPtr->StartLatLong = FVector2D(FileLat, FileLong);
	ContainerPtr->EndLatLong = ContainerPtr->StartLatLong + FVector2D(1.0f, 1.0f);

	// load the file data into the raw data array
	ContainerPtr->RawData.Reset(numValues);
	ContainerPtr->RawData.AddUninitialized(numValues);
	
	Archive->Serialize((void*)ContainerPtr->RawData.GetData(), Filesize);

	Archive->Close();

	return ContainerPtr;
}

USRTMContainer* FSRTMImport::LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong, EIncompleteDataHandling HandlingType /*= EIncompleteDataHandling::RETURN_NULLPTR*/) const
{
	return nullptr;
}

FString FSRTMImport::GetFilenameFromLatLong(int32 Lat, int32 Long) const
{
	return FString::Printf(TEXT("%s%.1d%s%.2d"), 
		Lat > 0 ? TEXT("N") : TEXT("S"),
		Lat,
		Long > 0 ? TEXT("E") : TEXT("W"),
		Long
	);
}

bool FSRTMImport::GetLatLongFromFilename(const FString& Filename, int32& Lat, int32& Long) const
{
	if (Filename.Len() < 7) return false;

	FString EW_Char = Filename.Right(3);

	int32 NS_Dir;
	int32 EW_Dir;

	if (Filename.StartsWith(TEXT("N")))
		NS_Dir = 1;
	else if (Filename.StartsWith("S"))
		NS_Dir = 0;
	else
		return false;

	if (EW_Char.StartsWith("E"))
		EW_Dir = 1;
	else if (EW_Char.StartsWith("W"))
		EW_Dir = 0;
	else
		return false;

	FString LatStr = Filename.Mid(1, 2);
	FString LongStr = Filename.Mid(4, 3);

	if (!FDefaultValueHelper::ParseInt(LatStr, Lat) ||
		!FDefaultValueHelper::ParseInt(LongStr, Long))
		return false;

	return true;
}
