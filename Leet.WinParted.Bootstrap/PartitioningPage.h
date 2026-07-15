#pragma once

#pragma once

#include <PantherLayout.h>
#include <PantherText.h>
#include <WinParted.h>

namespace Leet::WinParted::Frontend
{
	class PartitioningPage : public Leet::LibPanther::Layout::Page
	{
	public:
		PartitioningPage(Leet::Panther2K::Util::Console* console, PartitionManager* pm, const std::weak_ptr<PartitionManager::DiskPartitionTable>& partTable) : Page(console), m_partitionManager(pm), m_partitionTable(partTable) {}
		std::pair<Leet::LibPanther::Layout::PageResult, WP_PART_INFO> GetResult();
	protected:
		void InitPage() override;
		void CreateLayout() override;
		bool HandleKey(KEY_EVENT_RECORD* key) override;
	private:
		void LoadPartitionList() const;
		std::pair<Leet::LibPanther::Layout::PageResult, WP_PART_INFO> m_result;

		PartitionManager* m_partitionManager;
		std::weak_ptr<PartitionManager::DiskPartitionTable> m_partitionTable;

		Leet::LibPanther::TextUtils::ColumnFormatter m_formatter;
		std::shared_ptr<Leet::LibPanther::Layout::ListViewControl> m_listView = std::make_shared<Leet::LibPanther::Layout::ListViewControl>(-1);
	};
}