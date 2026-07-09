#include "ColumnFormatter.h"
#include <algorithm>

void Leet::LibPanther::TextUtils::ColumnFormatter::AddColumn(const std::wstring& header, int width, Alignment align)
{
	m_columns.push_back({ header, width, align });
}

std::vector<int> Leet::LibPanther::TextUtils::ColumnFormatter::CalculateWidths() const
{
	std::vector<int> widths(m_columns.size(), 0);
	int fixedWidthSum = 0;
	int stretchIndex = -1;

	for (size_t i = 0; i < m_columns.size(); ++i)
	{
		if (m_columns[i].width == 0)
			stretchIndex = (int)i;
		else
		{
			widths[i] = m_columns[i].width;
			fixedWidthSum += m_columns[i].width;
		}
	}

	int totalSpacing = m_spacing * std::max(0, (int)m_columns.size() - 1);
	if (stretchIndex != -1)
		widths[stretchIndex] = std::max(0, m_totalWidth - fixedWidthSum - totalSpacing);

	return widths;
}

std::wstring Leet::LibPanther::TextUtils::ColumnFormatter::FitString(const std::wstring& text, int targetWidth, Alignment align)
{
	if (text.length() >= (size_t)targetWidth)
		return text.substr(0, targetWidth);

	std::wstring padding(targetWidth - text.length(), L' ');
	return align == Alignment::Right ? padding + text : text + padding;
}

std::wstring Leet::LibPanther::TextUtils::ColumnFormatter::GetHeaderString() const
{
	std::vector<std::wstring> headers;
	for (const auto& col : m_columns)
		headers.push_back(col.header);

	return FormatRow(headers);
}

std::wstring Leet::LibPanther::TextUtils::ColumnFormatter::FormatRow(const std::vector<std::wstring>& rowData) const
{
	std::vector<int> widths = CalculateWidths();
	std::wstring result;
	std::wstring spaceStr(m_spacing, L' ');

	for (size_t i = 0; i < m_columns.size(); ++i)
	{
		std::wstring text = (i < rowData.size()) ? rowData[i] : L"";
		result += FitString(text, widths[i], m_columns[i].align);

		if (i < m_columns.size() - 1)
			result += spaceStr;
	}

	return result;
}