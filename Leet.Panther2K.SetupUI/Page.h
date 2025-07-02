#pragma once

#include <PantherConsole.h>

typedef enum {
	PageSuccess,
	PageContinue,
	PageGoBack,
	PageExit,
} PageResult;

class PopupPage;

class Page
{
public:
	Page();
	void Initialize(Leet::Panther2K::Util::Console* con);
	void Draw();
	void Redraw(bool redraw = true);
	PageResult HandleKey(WPARAM wParam);
	void AddPopup(PopupPage* popup);
	void RemovePopup();
	
	void DrawBox(int x, int y, int cx, int cy, bool useDouble);

	const wchar_t* text;
	const wchar_t* statusText;
	Leet::Panther2K::Util::Console* console;
private:
	virtual void Init();
	virtual void Drawer();
	virtual void Redrawer();
	virtual PageResult KeyHandler(WPARAM wParam);
protected:
	PopupPage* page;
};

