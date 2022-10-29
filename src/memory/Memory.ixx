export module Memory;

import Util;

import <array>;
import <concepts>;
import <string_view>;

export namespace Memory
{
	enum class Operation {
		Read, InstrFetch, Write
	};

	void Initialize();
	template<std::integral Int, Memory::Operation> Int ReadPhysical(u32 physical_address);
	void ReloadPageTables();
	template<size_t number_of_bytes> void WritePhysical(u32 physical_address, auto data);

	std::array<u8*, 0x10000> read_page_table{};
	std::array<u8*, 0x10000> write_page_table{};

	/// debugging
	std::string_view io_location;
}