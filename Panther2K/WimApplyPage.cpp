#include "WimApplyPage.h"

#include "MessageBoxPage.h"
#include <PantherLogger.h>

/*
DWORD __stdcall MessageCallback(IN DWORD Msg, IN WPARAM wParam, IN LPARAM lParam, IN PDWORD dwThreadId)
{
	wchar_t buffer[MAX_PATH * 2];
	switch (Msg)
	{
	case WIM_MSG_PROGRESS:
		swprintf_s(buffer, L"Installation progress: %lld%%", wParam);
		WriteToFile(buffer);
		PostThreadMessageW(*dwThreadId, WM_PROGRESSUPDATE, wParam, 0);
		break;
	case WIM_MSG_PROCESS:
		if (WindowsSetup::ShowFileNames && canSendFileName)
			if (PostThreadMessageW(*dwThreadId, WM_FILENAMEUPDATE, wParam, lParam))
				canSendFileName = false;
		break;
	case WIM_MSG_INFO:
	{
		wchar_t messageBuffer[MAX_PATH];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lParam, NULL, messageBuffer, MAX_PATH, NULL);
		swprintf_s(buffer, L"System message (File %s): %s", (wchar_t*)wParam, messageBuffer);
		WriteToFile(buffer);
		break;
	}
	case WIM_MSG_ERROR:
	{
		wchar_t messageBuffer[MAX_PATH];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lParam, NULL, messageBuffer, MAX_PATH, NULL);
		swprintf_s(buffer, L"Error (File %s): %s", (wchar_t*)wParam, messageBuffer);
		WriteToFile(buffer);
		break;
	}
	case WIM_MSG_WARNING:
	{
		wchar_t messageBuffer[MAX_PATH];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lParam, NULL, messageBuffer, MAX_PATH, NULL);
		swprintf_s(buffer, L"Warning (File %s): %s", (wchar_t*)wParam, messageBuffer);
		WriteToFile(buffer);
		break;
	}
	case WIM_MSG_TEXT:
		swprintf_s(buffer, L"%s\n", (wchar_t*)lParam);
		WriteToFile(buffer);
		break;
	case WIM_MSG_SETRANGE:
		swprintf_s(buffer, L"   Total number of files to be applied: %lld.", lParam);
		WriteToFile(buffer); 
		WriteToFile(L"Starting Windows installation...");
		break;
	case WIM_MSG_SETPOS:
		swprintf_s(buffer, L"Number of files applied: %lld.", lParam);
		WriteToFile(buffer);
		break;
	case WIM_MSG_QUERY_ABORT:
		WriteToFile(L"Abort opportunity given, but not aborting.");
		break;
	case WIM_MSG_METADATA_EXCLUDE:
	case WIM_MSG_STEPIT:
	case WIM_MSG_STEPNAME:
		break;
	default:
		swprintf_s(buffer, L"Unknown message: %d.", Msg);
		WriteToFile(buffer);
		break;
	}
	return WIM_MSG_SUCCESS;
}
*/

WimApplyPage::~WimApplyPage()
{
	if (statusText) free((wchar_t*)statusText);
}

void WimApplyPage::Update(int prog)
{
	progress = prog;
	Redraw();
}

LPCWSTR PathFindFileName(
	LPCWSTR pPath)
{
	LPCWSTR pT;

	for (pT = pPath; *pPath; pPath++) {
		if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':') || pPath[0] == TEXT('/'))
			&& pPath[1] && pPath[1] != TEXT('\\') && pPath[1] != TEXT('/'))
			pT = pPath + 1;
	}

	return pT;
}

void WimApplyPage::Update(const wchar_t* fileName)
{
	int length = console->GetSize().cx;
	int bufferSize = length + 1;
	int nameX = length - 25;
	/// | Copying: 12345678.123  

	wmemset((wchar_t*)statusText, L' ', bufferSize);
	wmemcpy_s((wchar_t*)statusText, bufferSize, L"  Panther2K is installing Windows...", 36);

	if (fileName)
	{
		wchar_t buffer[24];
		fileName = PathFindFileName(fileName);
		if (lstrlenW(fileName) > 8) swprintf_s(buffer, L"│ Copying: %.6s~0.%s", fileName, fileName + (lstrlenW(fileName) - 3));
		else swprintf_s(buffer, L"│ Copying: %.12s", fileName + (lstrlenW(fileName) - 3));
		wmemcpy_s((wchar_t*)statusText + nameX, bufferSize - nameX, buffer, 24);
	}
	else 
	{
		((wchar_t*)statusText)[36] = L'\x0';
	}

	((wchar_t*)statusText)[length] = L'\x0';
	_ASSERT(lstrlenW(statusText) <= length);
	Redraw();
}

void WimApplyPage::Init()
{
	statusText = (wchar_t*)safeMalloc(nullptr, sizeof(wchar_t) * (console->GetSize().cx + 1));
	if (!statusText) return;
	memcpy(((wchar_t*)statusText), L"  Panther2K is installing Windows...", 37 * sizeof(wchar_t));
}

void WimApplyPage::Drawer()
{
	SIZE consoleSize = console->GetSize();
	
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->DrawTextCenter(L"Please wait while Setup copies files to the Windows installation folders. This might take several minutes to complete.", consoleSize.cx / 3 * 2, 6);
	
	int boxWidth = consoleSize.cx - 12;
	int boxHeight = 7;
	int boxX = 6;
	int boxY = consoleSize.cy / 2 - 1;
	DrawBox(boxX, boxY, boxWidth, boxHeight, true);
	console->SetPosition(boxX + 2, boxY + 1);
	console->Write(L"Setup is copying files...");
	boxX += 6;
	boxY += 3;
	boxWidth -= 12;
	boxHeight -= 4;
	DrawBox(boxX, boxY, boxWidth, boxHeight, false);
}

void WimApplyPage::Redrawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	SIZE consoleSize = console->GetSize();
	int boxWidth = consoleSize.cx - 12;
	int boxX = 6;
	int boxY = consoleSize.cy / 2 - 1;

	wchar_t buffer[5];
	swprintf(buffer, 5, L"%i%%", progress);
	console->SetPosition(boxX + ((boxWidth / 2) - (4 / 2)), boxY + 2);
	console->Write(buffer);

	boxX += 6;
	boxY += 3;
	boxWidth -= 12;

	console->SetPosition(boxX + 1, boxY + 1);
	int progWidth = boxWidth - 2;
	int interpolatedProgress = progress * progWidth / 100;
	console->SetForegroundColor(CONSOLE_COLOR_PROGBAR);
	for (int i = 0; i < interpolatedProgress; i++)
		console->Write(L"█");
	for (int i = interpolatedProgress; i < progWidth; i++)
		console->Write(L" ");
}

bool WimApplyPage::KeyHandler(WPARAM wParam)
{
	return true;
}
