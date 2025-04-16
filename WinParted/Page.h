#pragma once
#include <PantherConsole.h>

namespace Leet 
{
	namespace WinParted 
	{
		class PartitionManager;
	}
}

class Page
{
public:
	Page();
	~Page();
	void Initialize(Leet::Panther2K::Util::Console* con);
	void Draw();
	void Update();
	void Run();
	void SetStatusText(const wchar_t* txt);
private:
	const wchar_t* text;
	const wchar_t* statusText;
	Leet::Panther2K::Util::Console* console;
	bool drawHeader;
	bool drawStatus;
	bool drawClear;

	friend class Leet::WinParted::PartitionManager;
protected:
	void SetDecorations(bool header, bool status, bool clear);
	void SetText(const wchar_t* txt);
	virtual void InitPage();
	virtual void DrawPage();
	virtual void UpdatePage();
	virtual void RunPage();
	Leet::Panther2K::Util::Console* GetConsole();
};

