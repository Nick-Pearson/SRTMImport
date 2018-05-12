#pragma once

#include "ISRTMImport.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SRTMImport.generated.h"


DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSRTMLoadCompleted_Dynamic, USRTMContainer*, DataObject);

// Blueprint Accessors
UCLASS()
class USRTMBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	* Loads the data from the provided filepath into an SRTM container, parses the Lat and Long data from the filename
	*
	* @return A valid container or nullptr if there was an error
	*/
	UFUNCTION(BlueprintCallable, Category = "SRTM", meta = (DisplayName = "LoadFile"))
	static USRTMContainer*	LoadFile(const FString& Filepath) { return ISRTMImport::Get().LoadFile(Filepath); }

	/**
	* Loads SRTM data from the required files in the folder specified in the plugin settings to populate the required region
	* NOTE: EndLat must be further west than StartLat, EndLong must be further north than StartLong
	*
	* Will call delegate with valid container or nullptr if there was an error
	*/
	UFUNCTION(BlueprintCallable, Category = "SRTM")
  static void	LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong, FOnSRTMLoadCompleted_Dynamic Delegate) { return ISRTMImport::Get().LoadRegion(StartLat, StartLong, EndLat, EndLong, FOnSRTMLoadCompleted::CreateLambda([&](USRTMContainer* DataObj) {Delegate.Execute(DataObj); }) ); }
};

struct FPendingRegionLoad
{
  FPendingRegionLoad(float inStartLat, float inStartLong, float inEndLat, float inEndLong, FOnSRTMLoadCompleted inDelegate) :
    StartLat(inStartLat), EndLat(inEndLat), StartLong(inStartLong), EndLong(inEndLong), Delegate(inDelegate)
  {}
  float StartLat, EndLat, StartLong, EndLong;
  FOnSRTMLoadCompleted Delegate;
  TArray<TSharedPtr<class IHttpRequest>> PendingHTTPRequests;
};


// iteratates over all tiles given a start and end lat and long
struct FTilesIterator
{
public:

  FTilesIterator(float inStartLat, float inEndLat, float inStartLong, float inEndLong) :
    EndLat(inEndLat), StartLong(inStartLong), EndLong(inEndLong)
  {
    fltLat = inStartLat;
    fltLong = inStartLong;
  }

  FORCEINLINE int32 Lat() const { return FMath::FloorToInt(fltLat); }
  FORCEINLINE int32 Long() const { return FMath::FloorToInt(fltLong); }

  FTilesIterator& operator++()
  {
    fltLong += 1.0f;

    if (fltLat > EndLong)
    {
      fltLong = StartLong;
      fltLat += 1.0f;
    }

    return *this;
  }

  // conversion to "bool" returning true if the iterator has not reached the last element.
  FORCEINLINE explicit operator bool() const
  {
    return fltLat < EndLat;
  }

private:

  float fltLat, fltLong;
  float EndLat;
  float StartLong, EndLong;
};


class FSRTMImport : public ISRTMImport
{
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;

public:
  virtual USRTMContainer* LoadFile(const FString& Filepath) const override;
  
  // loads a subregion from a single file in the saved dir
  virtual void LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong, FOnSRTMLoadCompleted Delegate) override;

  virtual bool IsValidFile(const TCHAR* Filename) const override;

private:

  // Parses SRTM data from a set of files
  USRTMContainer* ParseSRTMData(const TArray<FString>& Filepath, float StartLat, float StartLong, float EndLat, float EndLong) const;

  void OnTileDownloadComplete(TSharedPtr<class IHttpRequest> ReqPtr, TSharedPtr<class IHttpResponse, ESPMode::ThreadSafe> Response, bool Success, int32 Lat, int32 Long);
  void FinishRegionLoad(FPendingRegionLoad& Region);

  FString GetFilenameFromLatLong(int32 Lat, int32 Long) const;
  bool GetLatLongFromFilename(FString Filename, int32& Lat, int32& Long) const;

  FString GetSRTMDir() const { return FPaths::ProjectSavedDir() / "SRTM"; }

  TArray<FPendingRegionLoad> PendingRegionLoads;
};