#pragma once

#include <PantherConsole.h>
#include "Page.h"

class PopupPage
{
public:
	void Initialize(Console* con, Page* par);
	void Draw();
	PageResult HandleKey(WPARAM wParam);
	Page* parent;
private:
	virtual void Init();
	virtual void Drawer();
	virtual PageResult KeyHandler(WPARAM wParam);
protected:
	bool customColor = false;
	int fore, back;
	int width, height;
	const wchar_t* statusText;
	Console* console;
};
