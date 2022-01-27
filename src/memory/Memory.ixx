export module Memory;

import <array>;
import <cassert>;
import <functional>;

import Cartridge;
import NumericalTypes;

namespace Memory
{
	export Cartridge* cartridge;

	u64 read_pif(const u32 addr);
	u64 read_rom(const u32 addr);
	u64 invalid_read(const u32 addr);

	typedef u64(*read_fun_t)(const u32);

	constexpr std::array<read_fun_t, 0x1000> read_table = [] {
		std::array<read_fun_t, 0x1000> table{};
		unsigned addr = 0;
		for (auto& fun : table) {
			fun = [&] {
				if (addr <= 0x003) return invalid_read; /* temp */
				if (addr <= 0x007) return invalid_read;
				if (addr <= 0x03E) return invalid_read;
				if (addr == 0x03F) return invalid_read;
				if (addr == 0x040) return invalid_read;
				if (addr == 0x041) return invalid_read;
				if (addr == 0x042) return invalid_read;
				if (addr == 0x043) return invalid_read;
				if (addr == 0x044) return invalid_read;
				if (addr == 0x045) return invalid_read;
				if (addr == 0x046) return invalid_read;
				if (addr == 0x047) return invalid_read;
				if (addr == 0x048) return invalid_read;
				if (addr == 0x049) return invalid_read;
				if (addr <= 0x04F) return invalid_read;
				if (addr <= 0x05F) return invalid_read;
				if (addr <= 0x07F) return invalid_read;
				if (addr <= 0x0FF) return invalid_read;
				if (addr <= 0x1FB) return read_rom;
				if (addr == 0x1FC) return read_pif;
				if (addr <= 0x7FF) return invalid_read;
				return invalid_read;
			}();
			addr++;
		}
		return table;
	}();

	export
	template<typename T = u32 /*temp*/>
	T ReadPhysical(const u32 physical_address)
	{
		const auto page = physical_address >> 20;
		return std::invoke(read_table[page], physical_address);
	}

	u64 invalid_read(const u32 addr)
	{
		assert(false);
		return 0;
	}

	u64 read_pif(const u32 addr)
	{
		return 0;
	}

	u64 read_rom(const u32 addr)
	{
		return cartridge->read(addr);
	}
}