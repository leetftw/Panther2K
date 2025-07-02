#pragma once

#include <windows.h>

#define VKEY unsigned int

#define CONSOLE_COLOR_BG 0
#define CONSOLE_COLOR_FG 1
#define CONSOLE_COLOR_ERROR 2
#define CONSOLE_COLOR_PROGBAR 3
#define CONSOLE_COLOR_LIGHTFG 4
#define CONSOLE_COLOR_DARKFG 5

wchar_t* CleanString(const wchar_t* string);
wchar_t** SplitStringToLines(const wchar_t* string, int maxWidth, int* lineCount);

namespace Leet 
{
	namespace Panther2K 
	{
		namespace Util 
		{
			struct CONSOLE_COLOR
			{
				unsigned char R, G, B;

				COLORREF ToColor()
				{
					return RGB(R, G, B);
				}
			};

			class Console
			{
			public:
				Console();
				~Console();
				virtual bool Init();

				virtual void SetPosition(long x, long y);
				virtual POINT GetPosition();
				virtual void SetSize(long columns, long rows);
				virtual SIZE GetSize();

				void SetColors(CONSOLE_COLOR* colorTable);
				void SetBackgroundColor(CONSOLE_COLOR color);
				void SetBackgroundColor(int index);
				CONSOLE_COLOR GetBackgroundColor();
				void SetForegroundColor(CONSOLE_COLOR color);
				void SetForegroundColor(int index);
				CONSOLE_COLOR GetForegroundColor();

				void SetColorTable(CONSOLE_COLOR* colorTable, int colorTableSize);

				virtual void SetCursor(bool enabled, bool blinking);

				virtual void Write(const wchar_t* string);
				virtual void WriteLine(const wchar_t* string);
				virtual KEY_EVENT_RECORD* Read(int count = 1);
				virtual KEY_EVENT_RECORD* ReadLine();
				virtual void Update();
				virtual void Clear();

				void DrawBox(int boxX, int boxY, int boxWidth, int boxHeight, bool useDouble);
				void DrawTextLeft(const wchar_t* string, int cx, int y);
				void DrawTextRight(const wchar_t* string, int cx, int y);
				void DrawTextCenter(const wchar_t* string, int cx, int y);
			protected:
				int backColorIndex;
				int foreColorIndex;
				CONSOLE_COLOR backColor;
				CONSOLE_COLOR foreColor;
				CONSOLE_COLOR* colorTable;
				int colorTableSize;

				virtual void UpdateColorTable();
			};
		}
	}
}