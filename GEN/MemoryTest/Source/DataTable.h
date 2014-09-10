#pragma once

#include <string>
#include <vector>

class DataTable
{
private:
	std::vector<std::string> values;
	unsigned int numColumns;

public:
	DataTable(const std::vector<std::string>& headers);
	template <class ValType>
	void recordValue(unsigned int column, unsigned int row, ValType val)
	{
		recordValue(column, row, std::to_string(val));
	}
	template<> void recordValue(unsigned int column, unsigned int row, std::string val);
	template<> void recordValue(unsigned int column, unsigned int row, const char* val)
	{
		recordValue(column, row, std::string(val));
	}
	void printCSV(std::ostream& out);
};

template <>
void DataTable::recordValue(unsigned int column, unsigned int row, std::string val)
{
	if (column >= numColumns)
	{
		throw new std::exception("Column does not exist");
	}

	if (row >= values.size() / numColumns - 1)
	{
		values.resize((row + 2) * numColumns);
	}

	values[(row + 1) * numColumns + column] = val;
}
