#pragma once

#include "ISRTMImport.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SRTMImport.generated.h"

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
	* Loads the data from the provided filepath into an SRTM container using the provided Lat and Long
	*
	* @return A valid container or nullptr if there was an error
	*/
	UFUNCTION(BlueprintCallable, Category = "SRTM", meta = (DisplayName = "LoadFile"))
	static USRTMContainer*	LoadFile_ExplicitLatLong(const FString& Filepath, int32 FileLat, int32 FileLong) { return ISRTMImport::Get().LoadFile(Filepath, FileLat, FileLong); }

	/**
	* Loads SRTM data from the required files in the folder specified in the plugin settings to populate the required region
	* NOTE: EndLat must be further west than StartLat, EndLong must be further north than StartLong
	*
	* @return A valid container or nullptr if there was an error
	*/
	UFUNCTION(BlueprintCallable, Category = "SRTM")
	static USRTMContainer*	LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong) { return ISRTMImport::Get().LoadRegion(StartLat, StartLong, EndLat, EndLong); }
};


class FSRTMImport : public ISRTMImport
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	virtual USRTMContainer* LoadFile(const FString& Filepath) const override;
	virtual USRTMContainer* LoadFile(const FString& Filepath, int32 FileLat, int32 FileLong) const override;

	// loads a subregion from a single file in the saved dir
	virtual USRTMContainer* LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong) const override;

	virtual bool IsValidFile(const TCHAR* Filename) const override;

private:

	// Downloads / Finds all files required for the area required and returns the filepaths to them
	bool LoadFilesForArea(float StartLat, float StartLong, float EndLat, float EndLong) const;

	FString GetFilenameFromLatLong(int32 Lat, int32 Long) const;
	bool GetLatLongFromFilename(FString Filename, int32& Lat, int32& Long) const;
};