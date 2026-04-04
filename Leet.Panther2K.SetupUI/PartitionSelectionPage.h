#pragma once
#include "Page.h"
#include <vector>
#include "WinPartedDll.h"

class VolumeSelectionPage : public Page
{
public:
	VolumeSelectionPage(const wchar_t* fileSystem, long long minimumSize, long long minimumBytesAvailable, int stringIndex, int displayIndex);
	void SetVolumeList(VolumeInformation* volumes, int count);
	VolumeInformation GetSelectedVolume();
protected:
	virtual void Init() override;
	virtual void Drawer() override;
	virtual void Redrawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
private:
	int boxY = 0;
	int selectionIndex = 0;
	int scrollIndex = 0;
	std::vector<VolumeInformation> volumeInfo;
	int stringTableIndex;
	int dispIndex;
	bool showAll = false;
	struct
	{
		int partitionSize;
		int partitionFree;
		const wchar_t* fileSystem;
	} requirements;
};

