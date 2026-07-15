#include "DiskSelectionPage.h"

#include <PantherVersion.h>

#include "P2KBaseConsole.h"
#include "PartitioningPage.h"

extern void GetSizeStringFromBytes(unsigned long long bytes, wchar_t buffer[10]);

std::pair<Leet::LibPanther::Layout::PageResult, std::weak_ptr<Leet::WinParted::PartitionManager::Disk>> Leet::WinParted::Frontend::DiskSelectionPage::GetResult()
{
	//return m_partitionManager->OpenDisk();
	return m_result;
}

void Leet::WinParted::Frontend::DiskSelectionPage::InitPage()
{
	m_formatter.AddColumn(L"Disk", 4, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	m_formatter.AddColumn(L"Name", 0);
	m_formatter.AddColumn(L"# Part", 6, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	m_formatter.AddColumn(L"BlkSize", 7, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	m_formatter.AddColumn(L"Size", 9, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	
	m_result.first = Leet::LibPanther::Layout::PageExit;

	LoadDisks();
}

void Leet::WinParted::Frontend::DiskSelectionPage::CreateLayout()
{
	AddControl(std::make_shared<Leet::LibPanther::Layout::HeaderControl>(WINPARTED_VER_STRING L" (LibPanther Layout test)"));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::StatusBarControl>(L"WinParted still does not actually use this status bar."));
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"Welcome to WinParted", true));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"To begin partitioning, you need to select a disk first."));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To select a disk, use the UP and DOWN keys"));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To start partitioning the selected disk, press ENTER"));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To refresh the list of disks, press F5"));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To quit WinParted (and return to Panther2K) press F3"));
	Spacer(1);
	AddControl(m_listView);
}

bool Leet::WinParted::Frontend::DiskSelectionPage::HandleKey(KEY_EVENT_RECORD* key)
{
	switch (key->wVirtualKeyCode)
	{
		case VK_RETURN:
		{
			int selectedIndex = m_listView->GetSelectedIndex();
			if (selectedIndex >= 0 && selectedIndex < m_partitionManager->GetDiskCount())
			{
				m_result.first = Leet::LibPanther::Layout::PageContinue;
				m_result.second = m_partitionManager->OpenDisk(selectedIndex);
				return false;
			}
			break;
		}
	default:
		break;
	}
	return true;
}

void Leet::WinParted::Frontend::DiskSelectionPage::LoadDisks()
{
	m_formatter.SetTotalWidth(GetConsole()->GetSize().cx - 12);
	m_listView->ClearItems();
	m_listView->SetHeader(m_formatter.GetHeaderString());

	m_partitionManager->Refresh();
	for (int i = 0; i < m_partitionManager->GetDiskCount(); i++)
	{
		auto disk = m_partitionManager->OpenDisk(i);
		if (std::shared_ptr<PartitionManager::Disk> diskLock = disk.lock())
		{
			WP_DISK_INFO diskInfo = diskLock->GetDiskInfo();
			unsigned long long byteCount = diskInfo.SectorSize * diskInfo.SectorCount;
			wchar_t buffer[10];
			GetSizeStringFromBytes(byteCount, buffer);
			std::wstring formattedEntry = m_formatter.FormatRow
			(
				{
					std::to_wstring(diskInfo.DiskNumber),
					diskInfo.DeviceName,
					std::to_wstring(diskInfo.PartitionCount),
					std::to_wstring(diskInfo.SectorSize),
					buffer
				}
			);
			m_listView->AddItem(formattedEntry);
		}
		else
		{
			m_listView->AddItem(m_formatter.FormatRow({ L"", L"(failed to load)", L"", L"", L"" }));
			// TODO: add logger to Page class
		}
	}
}
