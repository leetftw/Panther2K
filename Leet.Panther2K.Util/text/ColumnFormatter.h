#pragma once

#pragma once
#include <string>
#include <vector>

namespace Leet::LibPanther::TextUtils
{
	class ColumnFormatter
	{
	public:
		enum class Alignment { Left, Right };

	private:
		struct Column
		{
			std::wstring header;
			int width;
			Alignment align;
		};

		std::vector<Column> m_columns;
		int m_totalWidth = 80;
		int m_spacing = 2;

		std::vector<int> CalculateWidths() const;
		static std::wstring FitString(const std::wstring& text, int targetWidth, Alignment align);

	public:
		ColumnFormatter() = default;

		void SetTotalWidth(int width) { m_totalWidth = width; }
		void SetSpacing(int space) { m_spacing = space; }
		void AddColumn(const std::wstring& header, int width, Alignment align = Alignment::Left);

		std::wstring GetHeaderString() const;
		std::wstring FormatRow(const std::vector<std::wstring>& rowData) const;
	};
}