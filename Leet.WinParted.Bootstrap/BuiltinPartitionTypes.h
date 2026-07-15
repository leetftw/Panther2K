#pragma once

#pragma once
#include <vector>

struct GUID;

namespace Leet::WinParted
{
	struct WP_PART_TYPE;

	class BuiltinPartitionTypes
	{
	public:
		static WP_PART_TYPE GetByGuid(GUID& guid);
		static WP_PART_TYPE GetByGpartedId(unsigned short gpartedId);
		static WP_PART_TYPE GetBySystemId(char systemId);
		static void GetAll(std::vector<WP_PART_TYPE>& list);
	private:
		static WP_PART_TYPE m_types[];
	};
}
