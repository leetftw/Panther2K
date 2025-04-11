#pragma once
#include "Page.h"
class BootPreparationPage : public Page
{
protected:
	virtual void Init() override;
	virtual void Drawer() override;
	virtual void Redrawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
};

