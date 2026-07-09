#pragma once
#include <windows.h>

namespace Leet::Panther2K::Util { class Console; }
namespace Leet::LibPanther::Layout
{
	class Page;

	class Control
	{
	protected:
		bool m_isFocusable = false;
		bool m_isFocused = false;
		int m_x = 0;
		int m_y = 0;
		int m_width = 0;
		int m_height = 1; // used for dynamic position controls
		int m_fixedHeight = 0; // used for fixed position controls (status bar only)
		friend class Page;
		void SetHeight(int newHeight) { m_height = newHeight; }
	public:
		virtual ~Control() = default;

		virtual void Draw(Leet::Panther2K::Util::Console* console) = 0;
		virtual bool HandleInput(KEY_EVENT_RECORD* key) { return false; }
		virtual void Update(Leet::Panther2K::Util::Console* console) {}

		bool IsFocusable() const { return m_isFocusable; }
		bool IsFocused() const { return m_isFocused; }
		void SetFocused(bool focused) { m_isFocused = focused; }

		int GetX() const { return m_x; }
		int GetY() const { return m_y; }
		int GetHeight() const { return m_height; }
		int GetFixedHeight() const { return m_fixedHeight; }

		void SetPosition(int newX, int newY) { m_x = newX; m_y = newY; }
	};
}