#include "PartitionManager.h"
#include <PantherConsole.h>
#include <PantherLayout.h>
#include <PantherVersion.h>
#include <PantherText.h>

static void GetSizeStringFromBytes(unsigned long long bytes, wchar_t buffer[10])
{
	// Overkill
	bool useSI = false;
	double factor = useSI ? 1000.0 : 1024.0;
	const wchar_t valueStringsSI[9][4] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB", L"ZB", L"YB" };
	const wchar_t valueStringsBinary[9][4] = { L"B", L"KiB", L"MiB", L"GiB", L"TiB", L"PiB", L"EiB", L"ZiB", L"YiB" };

	int index = 0;
	double bytesFp = bytes;
	while (bytesFp > factor)
	{
		bytesFp /= factor;
		index++;
	}

	swprintf(buffer, 10, L"%.1f%s", bytesFp, useSI ? valueStringsSI[index] : valueStringsBinary[index]);
}

int main()
{
	Leet::Panther2K::Util::Logger logger(L"debug.log", PANTHER_LL_VERBOSE);
	Leet::WinParted::PartitionManager partitionManager(&logger);
	Leet::Panther2K::Util::Win32Console console;
	Leet::LibPanther::TextUtils::ColumnFormatter formatter;
	
	console.Init();

	class CustomPage : public Leet::LibPanther::Layout::Page
	{
	public:
		CustomPage(Leet::Panther2K::Util::Console* console) : Page(console) {}
		std::shared_ptr<Leet::LibPanther::Layout::ListViewControl> listView = std::make_shared<Leet::LibPanther::Layout::ListViewControl>(-1);
	protected:
		void CreateLayout() override
		{
			AddControl(std::make_shared<Leet::LibPanther::Layout::HeaderControl>(WINPARTED_VER_STRING L" (LibPanther Layout test)"));
			AddControl(std::make_shared<Leet::LibPanther::Layout::StatusBarControl>(L"WinParted still does not actually use this status bar."));
			AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"Welcome to WinParted", true));
			AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"To begin partitioning, you need to select a disk first."));
			AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To select a disk, use the UP and DOWN keys"));
			AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To start partitioning the selected disk, press ENTER"));
			AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To refresh the list of disks, press F5"));
			AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To quit WinParted (and return to Panther2K) press F3"));
			AddControl(listView);
		}
	} page(&console);

	partitionManager.Refresh();

	formatter.AddColumn(L"Disk", 4, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	formatter.AddColumn(L"Name", 0);
	formatter.AddColumn(L"# Part", 6, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	formatter.AddColumn(L"BlkSize", 7, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	formatter.AddColumn(L"Size", 9, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);

	formatter.SetTotalWidth(console.GetSize().cx - 12);
	page.listView->SetHeader(formatter.GetHeaderString());

	for (int i = 0; i < partitionManager.GetDiskCount(); i++)
	{
		auto disk = partitionManager.OpenDisk(i);
		if (std::shared_ptr<Leet::WinParted::PartitionManager::Disk> diskLock = disk.lock())
		{
			Leet::WinParted::WP_DISK_INFO diskInfo = diskLock->GetDiskInfo();

			unsigned long long byteCount = diskInfo.SectorSize * diskInfo.SectorCount;
			wchar_t buffer[10];
			GetSizeStringFromBytes(byteCount, buffer);

			std::wstring formattedEntry = formatter.FormatRow
			(
				{
					std::to_wstring(diskInfo.DiskNumber),
					diskInfo.DeviceName,
					std::to_wstring(diskInfo.PartitionCount),
					std::to_wstring(diskInfo.SectorSize),
					buffer
				}
			);

			page.listView->AddItem(formattedEntry);
		}
		else
		{
			wprintf(L"Failed to lock disk %d!\n", i);
		}
	}

	page.Show();
}