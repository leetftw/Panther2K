#include "PartitioningPage.h"
#include <PantherConsole.h>

extern void GetSizeStringFromBytes(unsigned long long bytes, wchar_t buffer[10]);

std::pair<Leet::LibPanther::Layout::PageResult, WP_PART_INFO> Leet::WinParted::Frontend::PartitioningPage::GetResult()
{
	return m_result;
}

void Leet::WinParted::Frontend::PartitioningPage::InitPage()
{
	m_formatter.AddColumn(L"Part #", 6, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	m_formatter.AddColumn(L"Partition type", 0);
	m_formatter.AddColumn(L"Start", 6, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	m_formatter.AddColumn(L"End", 7, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);
	m_formatter.AddColumn(L"Size", 9, Leet::LibPanther::TextUtils::ColumnFormatter::Alignment::Right);

	m_result.first = Leet::LibPanther::Layout::PageExit;

	m_formatter.SetTotalWidth(GetConsole()->GetSize().cx - 12);
	m_listView->SetHeader(m_formatter.GetHeaderString());

	LoadPartitionList();
}

void Leet::WinParted::Frontend::PartitioningPage::CreateLayout()
{
	auto partTable = m_partitionTable.lock();
	if (!partTable)
	{
		// TODO: what the heck do you even do here
		return;
	}

	WP_DISK_INFO diskInfo = partTable->GetDiskInfo();

	AddControl(std::make_shared<Leet::LibPanther::Layout::HeaderControl>(WINPARTED_VER_STRING L" (LibPanther Layout test)"));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::StatusBarControl>(L"WinParted still does not actually use this status bar."));
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"Partitioning Disk %d. (%s)", true));

	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"The disk uses %s partitioning. (%s MBR)"));
	Spacer(1);

	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To select a partition, use the UP and DOWN keys"));
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To modify a partition, press ENTER"));
	Spacer(1);
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To create a new partition, press N"));
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To delete a partition, press D"));
	AddControl(std::make_shared<Leet::LibPanther::Layout::LabelControl>(L"   \x2022  To create an empty (new) partition table, press E"));
	Spacer(1);
	AddControl(m_listView);
	
}

bool Leet::WinParted::Frontend::PartitioningPage::HandleKey(KEY_EVENT_RECORD* key)
{
	auto partTable = m_partitionTable.lock();
	if (!partTable)
	{
		m_result.first = Leet::LibPanther::Layout::PageExit;
		return false;
	}

	switch (key->wVirtualKeyCode)
	{
	case VK_RETURN:
		break;
	case 'N':
		break;
	case 'D':
		partTable->DeletePartition(m_listView->GetSelectedIndex());
		LoadPartitionList();
		break;
	case 'E':
		break;
	case VK_F3:

		break;
	}

	return true;
}

void Leet::WinParted::Frontend::PartitioningPage::LoadPartitionList() const
{
	m_listView->ClearItems();

	if (auto partTable = m_partitionTable.lock())
	{
		for (int i = 0; i < partTable->GetPartitionCount(); ++i)
		{
			WP_PART_INFO partInfo;
			if (partTable->GetPartition(i, partInfo))
			{
				unsigned long long byteCount = partTable->GetDiskInfo().SectorSize * partInfo.SectorCount;
				wchar_t buffer[10];
				GetSizeStringFromBytes(byteCount, buffer);
				std::wstring formattedEntry = m_formatter.FormatRow({
					std::to_wstring(partInfo.PartitionNumber),
					partInfo.Type.Name,
					//L"(Not Implemented)",
					std::to_wstring(partInfo.StartLBA.ULL),
					std::to_wstring(partInfo.StartLBA.ULL),
					buffer
					});
				m_listView->AddItem(formattedEntry);
			}
			else
			{
				m_listView->AddItem(m_formatter.FormatRow({ L"", L"(failed to load)", L"", L"", L"" }));
			}
		}
	}
	else
	{
		m_listView->AddItem(L"Failed to load data");
	}
}
