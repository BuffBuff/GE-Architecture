#pragma once

#include <stdexcept>

namespace GENA
{
	class Buffer
	{
	private:
		char* bufData;
		size_t bufSize;

	public:
		Buffer();
		explicit Buffer(size_t size);
		Buffer(char* data, size_t size);
		Buffer(Buffer&& other);

		~Buffer();

		size_t size() const;
		void clear();

		char* data();
		const char* data() const;

		Buffer& operator=(Buffer&& other);

		char& operator[](size_t pos);
		char operator[](size_t pos) const;
		char& at(size_t pos);
		char at(size_t pos) const;

	private:
		Buffer(const Buffer&); // delete
		Buffer& operator=(const Buffer&); // delete
	};

	inline Buffer::Buffer()
		: bufData(nullptr),
		bufSize(0)
	{
	}

	inline Buffer::Buffer(size_t size)
		: bufData(new char[size]),
		bufSize(size)
	{
	}

	inline Buffer::Buffer(char* data, size_t size)
		: bufData(data),
		bufSize(size)
	{
	}

	inline Buffer::Buffer(Buffer&& other)
		: bufData(other.bufData),
		bufSize(other.bufSize)
	{
		other.bufData = nullptr;
		other.bufSize = 0;
	}

	inline Buffer::~Buffer()
	{
		if (bufData)
		{
			delete[] bufData;
			bufData = nullptr;
		}
	}

	inline size_t Buffer::size() const
	{
		return bufSize;
	}

	inline void Buffer::clear()
	{
		if (bufData)
		{
			delete[] bufData;
			bufData = nullptr;
			bufSize = 0;
		}
	}

	inline char* Buffer::data()
	{
		return bufData;
	}

	inline const char* Buffer::data() const
	{
		return bufData;
	}

	inline Buffer& Buffer::operator=(Buffer&& other)
	{
		std::swap(bufData, other.bufData);
		std::swap(bufSize, other.bufSize);
	}

	inline char& Buffer::operator[](size_t pos)
	{
		return bufData[pos];
	}

	inline char Buffer::operator[](size_t pos) const
	{
		return bufData[pos];
	}

	inline char& Buffer::at(size_t pos)
	{
		if (pos >= bufSize)
		{
			throw std::out_of_range("index out of range");
		}

		return bufData[pos];
	}

	inline char Buffer::at(size_t pos) const
	{
		if (pos >= bufSize)
		{
			throw std::out_of_range("index out of range");
		}

		return bufData[pos];
	}
}
