#pragma once
#include "Page.h"
class FinalPage : public Page
{
protected:
	virtual void Init() override;
	virtual void Drawer() override;
	virtual void Redrawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
private:
	int timer;
};

