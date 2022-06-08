export module Memory;

import MemoryAccess;
import NumericalTypes;

import <array>;
import <concepts>;
import <string_view>;

export namespace Memory
{
	void Initialize();

	template<std::integral Int, MemoryAccess::Operation operation>
	Int ReadPhysical(const u32 physical_address);

	template<std::size_t number_of_bytes>
	void WritePhysical(const u32 physical_address, const auto data);

	void ReloadPageTables();
}