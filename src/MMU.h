#pragma once

#include "NumericalTypes.h"

class MMU
{
public:
	enum class ReadFromNextBoundary
	{
		Yes, No
	};

	template<
		typename T,
		ReadFromNextBoundary read_from_next_boundary = ReadFromNextBoundary::No>
		T cpu_read_mem(u64 address)
	{
		return 0;
	}

	template<typename T>
	void cpu_write_mem(u64 address, T data)
	{

	}
};

