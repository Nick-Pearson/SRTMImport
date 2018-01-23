#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"


class USRTMContainer;

// indicates the behavior if not all requested data is available
enum class EIncompleteDataHandling : uint8
{
	// Do not return any container object
	RETURN_NULLPTR,

	// Return all data that is available inside a container object
	RETURN_AVAILIBLE,

};

/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class ISRTMImport : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ISRTMImport& Get()
	{
		return FModuleManager::LoadModuleChecked< ISRTMImport >( "SRTMImport" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "SRTMImport" );
	}

	/**
	* Loads the data from the provided filepath into an SRTM container, parses the Lat and Long data from the filename
	*
	* @return A valid container or nullptr if there was an error
	*/
	virtual USRTMContainer*	LoadFile(const FString& Filepath) const = 0;
	
	/**
	* Loads the data from the provided filepath into an SRTM container using the provided Lat and Long
	*
	* @return A valid container or nullptr if there was an error
	*/
	virtual USRTMContainer*	LoadFile(const FString& Filepath, int32 FileLat, int32 FileLong) const = 0;

	/**
	* Loads SRTM data from the required files in the folder specified in the plugin settings to populate the required region
	*
	* @return A valid container or nullptr if there was an error
	*/
	virtual USRTMContainer*	LoadRegion(float Lat, float Long, EIncompleteDataHandling HandlingType = EIncompleteDataHandling::RETURN_NULLPTR) const { return LoadRegion(Lat, Long, Lat + 1.0f, Long + 1.0f, HandlingType); }

	/**
	* Loads SRTM data from the required files in the folder specified in the plugin settings to populate the required region
	* NOTE: EndLat must be further west than StartLat, EndLong must be further north than StartLong
	*
	* @return A valid container or nullptr if there was an error
	*/
	virtual USRTMContainer*	LoadRegion(float StartLat, float StartLong, float EndLat, float EndLong, EIncompleteDataHandling HandlingType = EIncompleteDataHandling::RETURN_NULLPTR) const = 0;
};

