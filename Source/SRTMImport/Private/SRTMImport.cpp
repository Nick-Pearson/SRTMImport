#include "SRTMImport.h"

#include "SRTMContainer.h"
#include "Landscape/SRTMLandscapeHeightmap.h"

#include "FileManager.h"
#include "DefaultValueHelper.h"
#include "LandscapeEditorModule.h"


IMPLEMENT_MODULE( FSRTMImport, SRTMImport )


void FSRTMImport::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	FModuleManager::LoadModuleChecked<ILandscapeEditorModule>("LandscapeEditor").RegisterHeightmapFileFormat(MakeShareable(new SRTMLandscapeHeightmap));
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
	// todo allow for custom filepath
	return LoadRegion(FileLat, FileLong, FileLat + 1.0f, FileLong + 1.0f);

	/*TSharedPtr<FArchive> Archive = MakeShareable(IFileManager::Get().CreateFileReader(*Filepath));

	if (!Archive.IsValid())
		return nullptr;

	int64 Filesize = Archive->TotalSize();

	//ensure this is a valid file - must be factor of two and square number
	if (Filesize % 2 == 1) return nullptr;

	int32 numValues = Filesize / 2;
	float dimensionSize_flt = FMath::Sqrt((float)numValues);
	int32 dimensionSize = (int32)dimensionSize_flt;

	if (dimensionSize != dimensionSize_flt) return nullptr;

	// fill in the basic container info
	USRTMContainer* ContainerPtr = NewObject<USRTMContainer>();
	ContainerPtr->DimensionSize = FIntPoint(dimensionSize, dimensionSize);
	ContainerPtr->StartLatLong = FVector2D(FileLat, FileLong);
	ContainerPtr->EndLatLong = ContainerPtr->StartLatLong + FVector2D(1.0f, 1.0f);

	// load the file data into the raw data array
	ContainerPtr->RawData.Reset(numValues);
	ContainerPtr->RawData.AddUninitialized(numValues);
	
#if PLATFORM_LITTLE_ENDIAN
	// we have to swap each byte
	for (int32 i = 0; i < numValues; ++i)
	{
		char bytes[2];
		Archive->Serialize((void*)&bytes, 2);

		//swap the bytes
		char tmp = bytes[0];
		bytes[0] = bytes[1];
		bytes[1] = tmp;

		ContainerPtr->RawData[i] = *((uint16*)&bytes);
	}
#else
	Archive->Serialize((void*)ContainerPtr->RawData.GetData(), Filesize);
#endif

	Archive->Close();

	return ContainerPtr;*/
}

USRTMContainer* FSRTMImport::LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong) const
{
	if(StartLat > EndLat || StartLong > EndLong)
		return nullptr;

	if (!LoadFilesForArea(StartLat, StartLong, EndLat, EndLong))
		return nullptr;

	FString Filepath = FPaths::ProjectSavedDir() / "SRTM" / GetFilenameFromLatLong((int32)StartLat, (int32)StartLong);
	
	TSharedPtr<FArchive> Archive = MakeShareable(IFileManager::Get().CreateFileReader(*Filepath));

	if (!Archive.IsValid())
		return nullptr;

	int64 Filesize = Archive->TotalSize();

	//ensure this is a valid file - must be factor of two and square number
	if (Filesize % 2 == 1) return nullptr;

	int32 numValues = Filesize / 2;
	float dimensionSize_flt = FMath::Sqrt((float)numValues);
	int32 dimensionSize = (int32)dimensionSize_flt;

	if (dimensionSize != dimensionSize_flt) return nullptr;

	float LatOffset = StartLat - (int32)StartLat;
	int32 SeekLat = (int32)(LatOffset * dimensionSize);

	float LongOffset = StartLong - (int32)EndLong;
	int32 SeekLong = (int32)(LongOffset * dimensionSize);

	USRTMContainer* ContainerPtr = NewObject<USRTMContainer>();
	ContainerPtr->DimensionSize = FIntPoint(dimensionSize * (EndLat - StartLat), dimensionSize * (EndLong - StartLong));
	ContainerPtr->StartLatLong = FVector2D(StartLat, StartLong);
	ContainerPtr->EndLatLong = FVector2D(EndLat, EndLong);

	ContainerPtr->RawData.Reset(ContainerPtr->DimensionSize.X * ContainerPtr->DimensionSize.Y);
	ContainerPtr->RawData.AddUninitialized(ContainerPtr->DimensionSize.X * ContainerPtr->DimensionSize.Y);
	
	for (int32 LongIdx = 0; LongIdx < ContainerPtr->DimensionSize.Y; ++LongIdx)
	{
		// Seek to the correct location
		Archive->Seek((sizeof(uint16) * (SeekLong + LongIdx) * dimensionSize) + SeekLat);

#if PLATFORM_LITTLE_ENDIAN
		for (int32 LatIdx = 0; LatIdx < ContainerPtr->DimensionSize.X; ++LatIdx)
		{
			char bytes[2];
			Archive->Serialize((void*)&bytes, 2);

			//swap the bytes
			char tmp = bytes[0];
			bytes[0] = bytes[1];
			bytes[1] = tmp;

			ContainerPtr->RawData[(LongIdx * ContainerPtr->DimensionSize.X) + LatIdx] = *((uint16*)&bytes);
		}
#else
		Archive->Serialize((void*)(ContainerPtr->RawData.GetData() + (sizeof(uint16) * LongIdx * ContainerPtr->DimensionSize.X)), ContainerPtr->DimensionSize.X);
#endif
	}

	Archive->Close();

	return ContainerPtr;
}

bool FSRTMImport::IsValidFile(const TCHAR* Filename) const
{
	int32 Lat, Long;
	if (!GetLatLongFromFilename(FString(Filename), Lat, Long)) return false;

	return true;
}

bool FSRTMImport::LoadFilesForArea(float StartLat, float StartLong, float EndLat, float EndLong) const
{/*
	FString Filepath = FPaths::ProjectSavedDir() / "SRTM" / GetFilenameFromLatLong((int32)StartLat, (int32)StartLong);

	if (IFileManager::FileExists(*Filepath))
		Files.Add(Filepath);
		*/
	return true;
}

FString FSRTMImport::GetFilenameFromLatLong(int32 Lat, int32 Long) const
{
	return FString::Printf(TEXT("%s%.1d%s%.2d.hgt"), 
		Lat > 0 ? TEXT("N") : TEXT("S"),
		Lat,
		Long > 0 ? TEXT("E") : TEXT("W"),
		Long
	);
}

bool FSRTMImport::GetLatLongFromFilename(FString Filename, int32& Lat, int32& Long) const
{
	if (Filename.Len() > 11)
		Filename = Filename.RightChop(Filename.Len() - 11);

	if (Filename.Len() < 7) return false;

	FString EW_Char = Filename.RightChop(3);

	int32 NS_Dir;
	int32 EW_Dir;

	if (Filename.StartsWith(TEXT("N")))
		NS_Dir = 1;
	else if (Filename.StartsWith("S"))
		NS_Dir = -1;
	else
		return false;

	if (EW_Char.StartsWith("E"))
		EW_Dir = 1;
	else if (EW_Char.StartsWith("W"))
		EW_Dir = -1;
	else
		return false;

	FString LatStr = Filename.Mid(1, 2);
	FString LongStr = Filename.Mid(4, 3);

	if (!FDefaultValueHelper::ParseInt(LatStr, Lat) ||
		!FDefaultValueHelper::ParseInt(LongStr, Long))
		return false;

	Lat *= NS_Dir;
	Long *= EW_Dir;
	return true;
}
