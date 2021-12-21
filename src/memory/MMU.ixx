export module MMU;

import NumericalTypes;

namespace MMU
{
	export
	enum class ReadFromNextBoundary
	{
		Yes, No
	};

	export
	enum class WriteToNextBoundary
	{
		Yes, No
	};

	export
	template<typename T, ReadFromNextBoundary read_from_next_boundary = ReadFromNextBoundary::No>
	T cpu_read_mem(u64 address)
	{
		return T();
	}

	export
	template<typename T, WriteToNextBoundary write_to_next_boundary = WriteToNextBoundary::No>
	void cpu_write_mem(u64 address, T data) {

	}
}

