#pragma once
#include "Page.h"
#include "WinPartedDll.h"
#include <PantherLogger.h>

class DiskSelectionPage :
	public Page
{
public:
	~DiskSelectionPage();
	HRESULT LoadData(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger);
	int GetResult();
protected:
	virtual void Init() override;
	virtual void Drawer() override;
	virtual void Redrawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
private:
	int boxY = 0;
	int selectionIndex = 0;
	int scrollIndex = 0;
	int diskCount = 0;
	DISK_INFORMATION* diskInfo = 0;
};

