export module Memory;

import Util;

import <array>;
import <concepts>;
import <format>;
import <string_view>;

export namespace Memory
{
	enum class Operation {
		Read, InstrFetch, Write
	};

	void Initialize();
	void ReloadPageTables();

	template<std::signed_integral Int, Memory::Operation>
	Int ReadPhysical(u32 addr);

	template<size_t num_bytes>
	void WritePhysical(u32 addr, std::signed_integral auto data);

	std::array<u8*, 0x10000> read_page_table{};
	std::array<u8*, 0x10000> write_page_table{};

	//// debugging
	std::string_view io_location;
}