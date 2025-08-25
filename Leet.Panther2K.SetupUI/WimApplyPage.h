#pragma once
#include "Page.h"
class WimApplyPage : public Page
{
public:
	~WimApplyPage();
	void Update(int prog);
	void Update(const wchar_t* filename);
	void SetWarning(const wchar_t* warningText) { warning = warningText; Draw(); }
protected:
	virtual void Init() override;
	virtual void Drawer() override;
	virtual void Redrawer() override;
	virtual PageResult KeyHandler(WPARAM wParam) override;
private:
	int progress = 0;
	const wchar_t* warning = 0;
};

