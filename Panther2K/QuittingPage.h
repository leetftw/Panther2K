#pragma once

#include "PopupPage.h"

class QuittingPage : public PopupPage
{
private:
	virtual void Init() override;
	virtual void Drawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
};

