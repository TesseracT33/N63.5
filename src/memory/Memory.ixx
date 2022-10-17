export module Memory;

import MemoryAccess;
import Util;

import <array>;
import <concepts>;
import <string_view>;

export namespace Memory
{
	void Initialize();
	void ReloadPageTables();

	template<std::integral Int, MemoryAccess::Operation operation>
	Int ReadPhysical(u32 physical_address);

	template<size_t number_of_bytes>
	void WritePhysical(u32 physical_address, auto data);

	std::string_view io_location;

	std::array<u8*, 0x10000> read_page_table{};
	std::array<u8*, 0x10000> write_page_table{};
}