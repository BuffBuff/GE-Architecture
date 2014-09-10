#include "DataTable.h"

DataTable::DataTable(const std::vector<std::string>& headers)
	: values(headers), numColumns(headers.size())
{
}

void DataTable::printCSV(std::ostream& out)
{
	for (unsigned int row = 0; row < values.size() / numColumns; ++row)
	{
		out << values[row * numColumns];
		for (unsigned int col = 1; col < numColumns; ++col)
		{
			out << ';' << values[row * numColumns + col];
		}
		out << '\n';
	}
}
