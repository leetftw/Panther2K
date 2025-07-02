#pragma once
#include "Page.h"
class WimApplyPage : public Page
{
public:
	~WimApplyPage();
	void Update(int prog);
	void Update(const wchar_t* filename);
protected:
	virtual void Init() override;
	virtual void Drawer() override;
	virtual void Redrawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
private:
	int progress = 0;
};

