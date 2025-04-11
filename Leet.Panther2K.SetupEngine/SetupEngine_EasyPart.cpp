#include "SetupEngine.h"

/// <summary>
/// Sets the system volume by automatically detecting the boot partition
/// Only supported when running Windows 95
/// (Panther2K runs on NT only)
/// </summary>
/// <param name="volumeGuid">The volume GUID of the system partitions</param>
/// <returns>S_OK on success. Otherwise, it returns an HRESULT indicating what went wrong.</returns>
HRESULT Leet::Panther2K::SetupEngine::SetSystemVolumeEasy(const std::wstring& volumeGuid)
{
	// why the fuck did i create this file
	// this is winparted functionality not fucking pantherengine
	// the api exists anyway, lets just pretend it has a very specifc use case
	// and otherwise is not usable
	return E_ILLEGAL_METHOD_CALL;
}