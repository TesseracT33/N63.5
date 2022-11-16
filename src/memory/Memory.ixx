export module Memory;

import Util;

import <concepts>;
import <format>;

export namespace Memory
{
	enum class Operation {
		Read, InstrFetch, Write
	};

	void Initialize();

	template<std::signed_integral Int, Memory::Operation>
	Int ReadPhysical(u32 addr);

	template<size_t num_bytes>
	void WritePhysical(u32 addr, std::signed_integral auto data);
}