#pragma once
#include "Page.h"
#include <vector>
#include <string>

#include "../Leet.Panther2K.SetupEngine/engine_def.h"

class ImageSelectionPage : public Page
{
public:
	bool SetData(PantherWimInfo* wimInfo);
	int GetResult();
private:
	std::vector<std::wstring> FormattedStrings;
	int scrollIndex;
	int selectionIndex;
	int boxY;

	virtual void Init() override;
	virtual void Drawer() override;
	virtual void Redrawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
};

