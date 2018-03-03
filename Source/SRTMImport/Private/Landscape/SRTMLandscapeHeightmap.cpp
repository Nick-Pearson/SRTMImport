#include "SRTMLandscapeHeightmap.h"

#include "SRTMContainer.h"
#include "ISRTMImport.h"
#include "FileManager.h"

#define LOCTEXT_NAMESPACE "SRTMLandscapeHeightmap"

namespace
{
	const FText FileDescription(LOCTEXT("Description", "Real world heightmap data from the NASA Shuttle Radar Topography Mission"));
};

SRTMLandscapeHeightmap::SRTMLandscapeHeightmap()
{
	FileInfo.Description = FileDescription;
	FileInfo.Extensions = { ".hgt" };
	FileInfo.bSupportsExport = false;
}

FLandscapeHeightmapInfo SRTMLandscapeHeightmap::Validate(const TCHAR* HeightmapFilename) const
{
	FLandscapeHeightmapInfo Info;
	Info.ResultCode = ISRTMImport::Get().IsValidFile(HeightmapFilename) ? ELandscapeImportResult::Success : ELandscapeImportResult::Error;
	
	int64 FileSize = IFileManager::Get().FileSize(HeightmapFilename);
	int32 DataWidth = (int32)FMath::Sqrt(FileSize / 2);

	FLandscapeFileResolution Res;
	Res.Height = DataWidth;
	Res.Width = DataWidth;

	Info.PossibleResolutions = { Res };

	Info.DataScale = FVector(9.0f,9.0f,100.0f / 128.0f);

	return Info;
}

FLandscapeHeightmapImportData SRTMLandscapeHeightmap::Import(const TCHAR* HeightmapFilename, FLandscapeFileResolution ExpectedResolution) const
{
	FLandscapeHeightmapImportData Data;

	USRTMContainer* Container = ISRTMImport::Get().LoadFile(FString(HeightmapFilename));
	if (!Container)
	{
		Data.ResultCode = ELandscapeImportResult::Error;
		Data.ErrorMessage = LOCTEXT("FailedToLoadMsg", "Unable to load heights from SRTM file, ensure it is named correctly");
		return Data;
	}

	Data.ResultCode = ELandscapeImportResult::Success;
	Data.Data.Empty();
	Data.Data.AddUninitialized(Container->RawData.Num());

	for (int32 i = 0; i < Container->RawData.Num(); ++i)
	{
		Data.Data[i] = (uint16)(Container->RawData[i] + 32768);
	}

	return Data;
}

#undef LOCTEXT_NAMESPACE