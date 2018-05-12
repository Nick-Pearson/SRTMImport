#include "SRTMImport.h"

#include "SRTMContainer.h"
#include "Landscape/SRTMLandscapeHeightmap.h"

#include "FileManager.h"
#include "DefaultValueHelper.h"
#include "LandscapeEditorModule.h"

#include "HttpModule.h"
#include "IHttpResponse.h"


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

  return ParseSRTMData({ Filepath }, Lat, Long, Lat + 1.0f, Long + 1.0f);
}

void FSRTMImport::LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong, FOnSRTMLoadCompleted Delegate)
{
  if (!Delegate.IsBound())
    return;

	if(StartLat > EndLat || StartLong > EndLong)
		Delegate.ExecuteIfBound(nullptr);

  int32 NewItemIdx = PendingRegionLoads.Add(FPendingRegionLoad(StartLat, StartLong, EndLat, EndLong, Delegate));
  FPendingRegionLoad& PendingLoad = PendingRegionLoads[NewItemIdx];

  for (FTilesIterator It(StartLat, EndLat, StartLong, EndLong); It; ++It)
  {
    int32 Lat = It.Lat();
    int32 Long = It.Long();

    FString Filename = GetSRTMDir() / GetFilenameFromLatLong(Lat, Long);
    if(IFileManager::Get().FileExists(*Filename))
      continue;

    //load all files
    TSharedRef<IHttpRequest> HttpReq = FHttpModule::Get().CreateRequest();
    HttpReq->SetVerb("GET");
      
    HttpReq->SetURL(FString::Printf(TEXT("https://s3.amazonaws.com/elevation-tiles-prod/skadi/%s%.1d/%s%.1d%s%.3d.hgt.gz"),
      Lat > 0 ? TEXT("N") : TEXT("S"),
      FMath::Abs(Lat),
      Lat > 0 ? TEXT("N") : TEXT("S"),
      FMath::Abs(Lat),
      Long > 0 ? TEXT("E") : TEXT("W"),
      FMath::Abs(Long)));
      
    HttpReq->OnProcessRequestComplete().BindRaw(this, &FSRTMImport::OnTileDownloadComplete, Lat, Long);
      
    PendingLoad.PendingHTTPRequests.Add(HttpReq);

    HttpReq->ProcessRequest();
  }

  if (PendingLoad.PendingHTTPRequests.Num() == 0)
  {
    FinishRegionLoad(PendingLoad);
    PendingRegionLoads.RemoveAtSwap(NewItemIdx);
  }
}

bool FSRTMImport::IsValidFile(const TCHAR* Filename) const
{
	int32 Lat, Long;
	if (!GetLatLongFromFilename(FString(Filename), Lat, Long)) return false;

	return true;
}

USRTMContainer* FSRTMImport::ParseSRTMData(const TArray<FString>& Filepaths, float StartLat, float StartLong, float EndLat, float EndLong) const
{
  struct FDataFile
  {
    int32 Lat, Long;
    TSharedPtr<FArchive> Archive;
  };
  TArray<FDataFile> DataFiles;

  int32 DataFilesize = -1;

  //verify that we have all the files we require to parse this data
  for (FTilesIterator It(StartLat, EndLat, StartLong, EndLong); It; ++It)
  {
    int32 Lat = It.Lat();
    int32 Long = It.Long();

    FString Filename = GetFilenameFromLatLong(Lat, Long);
    FString Filepath = "";

    for (const FString& Path : Filepaths)
    {
      if (Path.Contains(Filename))
      {
        Filepath = Path;
        break;
      }
    }

    // Missing file error, return nullptr
    if (Filepath.Len() == 0)
    {
      UE_LOG(LogTemp, Warning, TEXT("Missing SRTM data file"));
      return nullptr;
    }

    FDataFile& DataFile = DataFiles[DataFiles.AddDefaulted(1)];
    DataFile.Lat = Lat;
    DataFile.Long = Long;

    DataFile.Archive = MakeShareable(IFileManager::Get().CreateFileReader(*Filepath));

    // Invalid filepath
    if (!DataFile.Archive.IsValid())
    {
      UE_LOG(LogTemp, Warning, TEXT("Failed to open SRTM datafile '%s'"), *Filepath);
      return nullptr;
    }

    int32 Filesize = DataFile.Archive->TotalSize();

    if (DataFilesize == -1) DataFilesize = Filesize;
    else if (DataFilesize != Filesize)
    {
      UE_LOG(LogTemp, Warning, TEXT("Mismatched resolutions in SRTM data files"));
      return nullptr;
    }
  }

  //ensure this is a valid file - must be factor of two and square number
  if (DataFilesize % 2 == 1)
  {
    UE_LOG(LogTemp, Warning, TEXT("One or more corrupted SRTM data files (incorrect number of values)"));
    return nullptr;
  }

  const int32 numValues = DataFilesize / 2;
  const float dimensionSize_flt = FMath::Sqrt((float)numValues);
  const int32 dimensionSize = (int32)dimensionSize_flt;

  if (dimensionSize != dimensionSize_flt)
  {
    UE_LOG(LogTemp, Warning, TEXT("One or more corrupted SRTM data files (incorrect number of values)"));
    return nullptr;
  }

  USRTMContainer* ContainerPtr = NewObject<USRTMContainer>();
  ContainerPtr->DimensionSize = FIntPoint(dimensionSize * (EndLong - StartLong), dimensionSize * (EndLat - StartLat));
  ContainerPtr->StartLatLong = FVector2D(StartLat, StartLong);
  ContainerPtr->EndLatLong = FVector2D(EndLat, EndLong);

  ContainerPtr->RawData.AddUninitialized(ContainerPtr->DimensionSize.X * ContainerPtr->DimensionSize.Y);

  // copy values out of files one at a time
  for (const FDataFile& DataFile : DataFiles)
  {
    const float FileStartLat = FMath::Max((float)DataFile.Lat, StartLat);
    const float FileStartLong = FMath::Max((float)DataFile.Long, StartLong);
    const float FileEndLat = FMath::Min((float)DataFile.Lat + 1.0f, EndLat);
    const float FileEndLong = FMath::Min((float)DataFile.Long + 1.0f, EndLong);

    const FIntPoint FileDimensionSize = FIntPoint(dimensionSize * (FileEndLong - FileStartLong), dimensionSize * (FileEndLat - FileStartLat));

    const int32 SeekLong = (int32)((FileStartLong - FMath::FloorToFloat(FileStartLong)) * dimensionSize);
    const int32 SeekLat = (int32)((FileStartLat - FMath::FloorToFloat(FileStartLat)) * dimensionSize);

    for (int32 LatIdx = FileDimensionSize.Y-1; LatIdx >= 0; --LatIdx)
    {
      // Seek to the correct location
      DataFile.Archive->Seek(sizeof(int16) * (((dimensionSize - (SeekLat + LatIdx)) * (dimensionSize + 1)) + SeekLong));

      for (int32 LongIdx = 0; LongIdx < FileDimensionSize.X; ++LongIdx)
      {
        union
        {
          uint8 bytes[2];
          int16 intVal;
        } u;
        DataFile.Archive->Serialize((void*)(&u.bytes), 2);

#if PLATFORM_LITTLE_ENDIAN
        //swap the bytes
        Swap(u.bytes[0], u.bytes[1]);
#endif

        ContainerPtr->RawData[(LatIdx * ContainerPtr->DimensionSize.X) + LongIdx] = u.intVal;
      }
    }
    
    DataFile.Archive->Close();
  }

  return ContainerPtr;
}

void FSRTMImport::OnTileDownloadComplete(TSharedPtr<IHttpRequest> ReqPtr, TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response, bool Success, int32 Lat, int32 Long)
{
  int32 ItemIdx = PendingRegionLoads.FindLastByPredicate([&](const FPendingRegionLoad& QueryRegion) {
    return QueryRegion.PendingHTTPRequests.Contains(ReqPtr);
  });

  if (ItemIdx == INDEX_NONE)
    return;

  FPendingRegionLoad& PendingLoadPtr = PendingRegionLoads[ItemIdx];

  if (!Success || Response->GetResponseCode() != 200)
  {
    UE_LOG(LogTemp, Error, TEXT("Error while downloading SRTM tiles, check your internet connection"));
    PendingLoadPtr.Delegate.ExecuteIfBound(nullptr);
    PendingRegionLoads.RemoveAtSwap(ItemIdx);
    return;
  }

  //determine the uncompressed size
  const TArray<uint8>& Content = Response->GetContent();
  int32 UCSIdx = Content.Num() - 4;
#if PLATFORM_LITTLE_ENDIAN
  int32 UncompressedSize = (Content[UCSIdx + 3] << 24) | (Content[UCSIdx + 2] << 16) + (Content[UCSIdx + 1] << 8) + Content[UCSIdx];
#else
  int32 UncompressedSize = (Content[UCSIdx] << 24) | (Content[UCSIdx + 1] << 16) + (Content[UCSIdx + 2] << 8) + Content[UCSIdx + 3];
#endif

  TArray<uint8> UncompressedContent;
  UncompressedContent.AddUninitialized(UncompressedSize);

  if (!ensure(FCompression::UncompressMemory(COMPRESS_ZLIB, (void*)UncompressedContent.GetData(), UncompressedSize, (const void*)Content.GetData(), Content.Num(), false, DEFAULT_ZLIB_BIT_WINDOW | 16)))
  {
    UE_LOG(LogTemp, Error, TEXT("Failed to decompress SRTM tile"));
    return;
  }
  
  FString FilePath = GetSRTMDir() / GetFilenameFromLatLong(Lat, Long);
  TSharedPtr<FArchive> FileWrite = MakeShareable(IFileManager::Get().CreateFileWriter(*FilePath));

  FileWrite->Serialize((void*)UncompressedContent.GetData(), UncompressedContent.Num());

  FileWrite->Close();

  PendingLoadPtr.PendingHTTPRequests.RemoveSwap(ReqPtr);

  if (PendingLoadPtr.PendingHTTPRequests.Num() == 0)
  {
    FinishRegionLoad(PendingLoadPtr);
    PendingRegionLoads.RemoveAtSwap(ItemIdx);
  }
}

void FSRTMImport::FinishRegionLoad(FPendingRegionLoad& Region)
{
  TArray<FString> Filepaths;

  for (FTilesIterator It(Region.StartLat, Region.EndLat, Region.StartLong, Region.EndLong); It; ++It)
  {
    Filepaths.Add(GetSRTMDir() / GetFilenameFromLatLong(It.Lat(), It.Long()));
  }

  Region.Delegate.ExecuteIfBound(ParseSRTMData(Filepaths, Region.StartLat, Region.StartLong, Region.EndLat, Region.EndLong));
}

FString FSRTMImport::GetFilenameFromLatLong(int32 Lat, int32 Long) const
{
	return FString::Printf(TEXT("%s%.1d%s%.2d.hgt"), 
		Lat > 0 ? TEXT("N") : TEXT("S"),
    FMath::Abs(Lat),
		Long > 0 ? TEXT("E") : TEXT("W"),
		FMath::Abs(Long)
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
