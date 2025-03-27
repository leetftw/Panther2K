#pragma once

#include <PantherConsole.h>
#include "PopupPage.h"

/*
typedef enum {
	Success,
	Continue,
	GoBack,
	Fail,
	RunTool,
} PageResult;
*/

class Page
{
public:
	Page();
	void Initialize(Console* con);
	void Draw();
	void Redraw(bool redraw = true);
	bool HandleKey(WPARAM wParam);
	void AddPopup(PopupPage* popup);
	void RemovePopup();
	
	void DrawBox(int x, int y, int cx, int cy, bool useDouble);

	const wchar_t* text;
	const wchar_t* statusText;
	Console* console;
private:
	virtual void Init();
	virtual void Drawer();
	virtual void Redrawer();
	virtual bool KeyHandler(WPARAM wParam);
protected:
	PopupPage* page;
};

