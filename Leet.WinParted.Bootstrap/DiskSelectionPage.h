#pragma once

#include <PantherLayout.h>
#include <PantherText.h>
#include <WinParted.h>

namespace Leet::WinParted::Frontend
{
	class DiskSelectionPage : public Leet::LibPanther::Layout::Page
	{
	public:
		DiskSelectionPage(Leet::Panther2K::Util::Console* console, Leet::WinParted::PartitionManager* pm) : Page(console), m_partitionManager(pm) {}
		std::pair<Leet::LibPanther::Layout::PageResult, std::weak_ptr<PartitionManager::Disk>> GetResult();
	protected:
		void InitPage() override;
		void CreateLayout() override;
		bool HandleKey(KEY_EVENT_RECORD* key) override;
	private:
		void LoadDisks();
		std::pair<Leet::LibPanther::Layout::PageResult, std::weak_ptr<PartitionManager::Disk>> m_result;

		PartitionManager* m_partitionManager;
		Leet::LibPanther::TextUtils::ColumnFormatter m_formatter;
		std::shared_ptr<Leet::LibPanther::Layout::ListViewControl> m_listView = std::make_shared<Leet::LibPanther::Layout::ListViewControl>(-1);
	};
}