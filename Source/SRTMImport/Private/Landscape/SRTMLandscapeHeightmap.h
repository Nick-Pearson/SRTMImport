#pragma once

#include "CoreMinimal.h"
#include "LandscapeFileFormatInterface.h"


class SRTMLandscapeHeightmap : public ILandscapeHeightmapFileFormat
{
public:
	SRTMLandscapeHeightmap();

	virtual const FLandscapeFileTypeInfo& GetInfo() const override { return FileInfo; }

	virtual FLandscapeHeightmapInfo Validate(const TCHAR* HeightmapFilename) const override;

	virtual FLandscapeHeightmapImportData Import(const TCHAR* HeightmapFilename, FLandscapeFileResolution ExpectedResolution) const override;

private:
	FLandscapeFileTypeInfo FileInfo;
};
